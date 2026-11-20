#include "NarrativeChoiceSolve.h"

namespace NarrativeChoice
{
	/*
	 * Named, not anonymous: in a unity build this file shares a translation unit with NarrativeChoice.cpp, whose helpers
	 * (MinSigma, LogSumExp, PickPhrasing, LeadPhrasing) sit in NarrativeChoice's anonymous namespace. Nothing here may
	 * reuse those names, since the using-directive below makes these visible wherever NarrativeChoice is reopened.
	 */
	namespace SolveDetail
	{
		/* Matches NarrativeChoice.cpp's MinSigma: an authored σ of 0 means "this exact spot". */
		constexpr double SolveMinSigma = 1e-3;

		/* How close counts as met: utility units for the terms, absolute probability for P. */
		constexpr double TermTolerance = 1e-3;
		constexpr double ProbabilityTolerance = 1e-4;

		/* Bisection steps: each halves the bracket, so 80 is far past double precision for any sane bracket. */
		constexpr int32 BisectSteps = 80;

		constexpr double SolveTiny = 1e-12;
		constexpr double SolveInfinity = TNumericLimits<double>::Max();

		double SolveSigma2(const FPhrasing& Phrasing)
		{
			return FMath::Square(FMath::Max(Phrasing.Sigma, SolveMinSigma));
		}

		bool IsFree(TConstArrayView<bool> Locked, int32 Axis)
		{
			return !(Locked.IsValidIndex(Axis) && Locked[Axis]);
		}

		int32 CountFree(TConstArrayView<bool> Locked, int32 Axes)
		{
			int32 Free = 0;
			for (int32 Axis = 0; Axis < Axes; ++Axis)
			{
				Free += IsFree(Locked, Axis) ? 1 : 0;
			}
			return Free;
		}

		double SqDistance(TConstArrayView<double> A, TConstArrayView<double> B)
		{
			double Sum = 0.0;
			for (int32 Axis = 0; Axis < A.Num(); ++Axis)
			{
				Sum += FMath::Square(A[Axis] - B[Axis]);
			}
			return Sum;
		}

		TArray<double> Difference(TConstArrayView<double> A, TConstArrayView<double> B)
		{
			TArray<double> Out;
			Out.SetNum(A.Num());
			for (int32 Axis = 0; Axis < A.Num(); ++Axis)
			{
				Out[Axis] = A[Axis] - B[Axis];
			}
			return Out;
		}

		double ResonanceAt(const FPhrasing& Phrasing, TConstArrayView<double> Location)
		{
			return -SqDistance(Phrasing.SpeakerX, Location) / SolveSigma2(Phrasing);
		}

		double RelevanceAt(const FParams& Params, const FPhrasing& Phrasing, TConstArrayView<double> Location)
		{
			const double Beta = FMath::Max(Params.Beta, UE_DOUBLE_SMALL_NUMBER);
			return LogExpectedReachPow(Phrasing.BeliefMean, Phrasing.BeliefVariance, Location, Phrasing.Sigma, Beta * Params.Eta) / Beta;
		}

		double UtilityAt(const FParams& Params, const FPhrasing& Phrasing, double Affinity, TConstArrayView<double> Location)
		{
			// UtilityBias is constant in the card's location, but it is part of u_y, so every solve must carry it.
			return ResonanceAt(Phrasing, Location) + RelevanceAt(Params, Phrasing, Location) + Params.Lambda * Phrasing.Stance * Affinity
				+ Phrasing.UtilityBias;
		}

		/* w_i: how fast relevance falls per squared unit off the belief on axis i. Relevance = ceiling - Σ w_i (μ_i - l_i)². */
		TArray<double> RelevanceWeights(const FParams& Params, const FPhrasing& Phrasing)
		{
			const double Beta = FMath::Max(Params.Beta, UE_DOUBLE_SMALL_NUMBER);
			const double A = Beta * Params.Eta / SolveSigma2(Phrasing);
			TArray<double> Out;
			Out.SetNum(Phrasing.Location.Num());
			for (int32 Axis = 0; Axis < Out.Num(); ++Axis)
			{
				Out[Axis] = A / (Beta * (1.0 + 2.0 * A * FMath::Max(0.0, Phrasing.BeliefVariance[Axis])));
			}
			return Out;
		}

		/* Relevance with the card on the belief: the term's ceiling. */
		double RelevanceCeiling(const FParams& Params, const FPhrasing& Phrasing)
		{
			return RelevanceAt(Params, Phrasing, Phrasing.BeliefMean);
		}

		double ResonanceSqRadius(const FPhrasing& Phrasing, double Value)
		{
			return -Value * SolveSigma2(Phrasing);
		}

		double RelevanceQ(const FParams& Params, const FPhrasing& Phrasing, double Value)
		{
			return RelevanceCeiling(Params, Phrasing) - Value;
		}

		/* The squared radius of the ball about the speaker where K ≥ ReachFloor; negative when every location is in reach. */
		double ReachSqRadius(const FParams& Params, const FPhrasing& Phrasing)
		{
			if (Params.ReachFloor <= 0.0)
			{
				return -1.0;
			}
			return Params.ReachFloor >= 1.0 ? 0.0 : -FMath::Loge(Params.ReachFloor) * SolveSigma2(Phrasing);
		}

		double SolveLogSumExp(TConstArrayView<double> Values)
		{
			double Max = -SolveInfinity;
			for (const double Value : Values)
			{
				Max = FMath::Max(Max, Value);
			}
			if (Values.IsEmpty() || Max == -SolveInfinity)
			{
				return -SolveInfinity;
			}
			double Sum = 0.0;
			for (const double Value : Values)
			{
				Sum += FMath::Exp(Value - Max);
			}
			return Max + FMath::Loge(Sum);
		}

		bool IsProbability(ESolveTarget Target)
		{
			return Target == ESolveTarget::CardProbability || Target == ESolveTarget::FamilyProbability;
		}

		FString FormatValue(ESolveTarget Target, double Value)
		{
			return IsProbability(Target) ? FString::Printf(TEXT("%.2f%%"), 100.0 * Value) : FString::Printf(TEXT("%.3f"), Value);
		}

		/* A unit step on the free axes along Preferred, or along the first free axis when Preferred has none there. */
		TArray<double> FreeDirection(TConstArrayView<double> Preferred, TConstArrayView<bool> Locked, int32 Axes)
		{
			TArray<double> Out;
			Out.Init(0.0, Axes);
			double Length = 0.0;
			for (int32 Axis = 0; Axis < Axes; ++Axis)
			{
				if (IsFree(Locked, Axis) && Preferred.IsValidIndex(Axis))
				{
					Out[Axis] = Preferred[Axis];
					Length += FMath::Square(Preferred[Axis]);
				}
			}
			if (Length > UE_DOUBLE_SMALL_NUMBER)
			{
				Length = FMath::Sqrt(Length);
				for (double& Value : Out)
				{
					Value /= Length;
				}
				return Out;
			}
			for (int32 Axis = 0; Axis < Axes; ++Axis)
			{
				if (IsFree(Locked, Axis))
				{
					Out[Axis] = 1.0;
					break;
				}
			}
			return Out;
		}

		/* A free unit vector orthogonal to every vector in Span (each already zero on locked axes); false when the free axes are all spanned. */
		bool FreePerpendicular(TConstArrayView<TArray<double>> Span, TConstArrayView<bool> Locked, int32 Axes, TArray<double>& Out)
		{
			// Gram-Schmidt the span, then the free unit axes against it, keeping the first that survives.
			TArray<TArray<double>> Basis;
			for (const TArray<double>& Vector : Span)
			{
				TArray<double> V = Vector;
				for (const TArray<double>& B : Basis)
				{
					double Dot = 0.0;
					for (int32 Axis = 0; Axis < Axes; ++Axis)
					{
						Dot += V[Axis] * B[Axis];
					}
					for (int32 Axis = 0; Axis < Axes; ++Axis)
					{
						V[Axis] -= Dot * B[Axis];
					}
				}
				double Length = 0.0;
				for (const double Value : V)
				{
					Length += Value * Value;
				}
				if (Length > 1e-20)
				{
					Length = FMath::Sqrt(Length);
					for (double& Value : V)
					{
						Value /= Length;
					}
					Basis.Add(MoveTemp(V));
				}
			}
			for (int32 Candidate = 0; Candidate < Axes; ++Candidate)
			{
				if (!IsFree(Locked, Candidate))
				{
					continue;
				}
				TArray<double> V;
				V.Init(0.0, Axes);
				V[Candidate] = 1.0;
				for (const TArray<double>& B : Basis)
				{
					const double Dot = B[Candidate];
					for (int32 Axis = 0; Axis < Axes; ++Axis)
					{
						V[Axis] -= Dot * B[Axis];
					}
				}
				double Length = 0.0;
				for (const double Value : V)
				{
					Length += Value * Value;
				}
				if (Length > 0.01)
				{
					Length = FMath::Sqrt(Length);
					for (double& Value : V)
					{
						Value /= Length;
					}
					Out = MoveTemp(V);
					return true;
				}
			}
			return false;
		}

		/*
		 * The nearest point to Location with ||Center - l||² = SqRadius, moving only the free axes. The locked axes'
		 * share of the distance is fixed, so the free part makes up the rest; false when even zero free distance is too far.
		 */
		bool ProjectSphere(TConstArrayView<double> Center, TArray<double>& Location, double SqRadius, TConstArrayView<bool> Locked, TConstArrayView<double> Fallback)
		{
			double LockedSq = 0.0;
			double FreeSq = 0.0;
			for (int32 Axis = 0; Axis < Location.Num(); ++Axis)
			{
				(IsFree(Locked, Axis) ? FreeSq : LockedSq) += FMath::Square(Location[Axis] - Center[Axis]);
			}
			const double Need = SqRadius - LockedSq;
			const bool bReachable = Need >= -UE_DOUBLE_KINDA_SMALL_NUMBER;
			const double Radius = FMath::Sqrt(FMath::Max(0.0, Need));

			if (FreeSq > UE_DOUBLE_SMALL_NUMBER)
			{
				const double Scale = Radius / FMath::Sqrt(FreeSq);
				for (int32 Axis = 0; Axis < Location.Num(); ++Axis)
				{
					if (IsFree(Locked, Axis))
					{
						Location[Axis] = Center[Axis] + (Location[Axis] - Center[Axis]) * Scale;
					}
				}
			}
			else
			{
				// Sitting on the centre: every direction is equally near, so take the caller's preference.
				const TArray<double> Direction = FreeDirection(Fallback, Locked, Location.Num());
				for (int32 Axis = 0; Axis < Location.Num(); ++Axis)
				{
					if (IsFree(Locked, Axis))
					{
						Location[Axis] = Center[Axis] + Direction[Axis] * Radius;
					}
				}
			}
			return bReachable;
		}

		/*
		 * The nearest point to Location with Σ w_i (l_i - Center_i)² = Q on the free axes plus the locked axes' fixed share.
		 * Lagrange gives l_i - c_i = d_i / (1 + κ w_i); the constraint is monotone in κ on (-1/w_max, ∞), so κ is found by
		 * bisection. Two cases sit outside that: the card on the centre, where the nearest point is the end of the shortest
		 * semi-axis; and no offset on the most-weighted axes when moving outward, where κ = -1/w_max exactly and those axes
		 * take up whatever the others leave.
		 */
		bool ProjectEllipsoid(TConstArrayView<double> Center, TConstArrayView<double> Weights, TArray<double>& Location, double Q, TConstArrayView<bool> Locked,
			TConstArrayView<double> Fallback)
		{
			double LockedQ = 0.0;
			double FreeQ = 0.0;
			double MaxWeight = 0.0;
			int32 MaxAxis = INDEX_NONE;
			for (int32 Axis = 0; Axis < Location.Num(); ++Axis)
			{
				const double Term = Weights[Axis] * FMath::Square(Location[Axis] - Center[Axis]);
				if (!IsFree(Locked, Axis))
				{
					LockedQ += Term;
					continue;
				}
				FreeQ += Term;
				// The most-weighted free axis; among equals, the one the caller leans along most.
				const bool bHeavier = Weights[Axis] > MaxWeight * (1.0 + 1e-9);
				const bool bTied = !bHeavier && Weights[Axis] >= MaxWeight * (1.0 - 1e-9) && MaxAxis != INDEX_NONE
					&& FMath::Abs(Fallback.IsValidIndex(Axis) ? Fallback[Axis] : 0.0) > FMath::Abs(Fallback.IsValidIndex(MaxAxis) ? Fallback[MaxAxis] : 0.0);
				if (bHeavier || bTied || MaxAxis == INDEX_NONE)
				{
					MaxWeight = FMath::Max(MaxWeight, Weights[Axis]);
					MaxAxis = Axis;
				}
			}
			const double Need = Q - LockedQ;
			const bool bReachable = Need >= -UE_DOUBLE_KINDA_SMALL_NUMBER;
			const double Target = FMath::Max(0.0, Need);
			if (MaxWeight <= UE_DOUBLE_SMALL_NUMBER)
			{
				return Target <= UE_DOUBLE_KINDA_SMALL_NUMBER;
			}
			const double Sign = Fallback.IsValidIndex(MaxAxis) && Fallback[MaxAxis] < 0.0 ? -1.0 : 1.0;

			if (FreeQ <= UE_DOUBLE_SMALL_NUMBER)
			{
				for (int32 Axis = 0; Axis < Location.Num(); ++Axis)
				{
					if (IsFree(Locked, Axis))
					{
						Location[Axis] = Center[Axis];
					}
				}
				Location[MaxAxis] = Center[MaxAxis] + Sign * FMath::Sqrt(Target / MaxWeight);
				return bReachable;
			}

			const TArray<double> Offset = Difference(Location, Center);
			auto Constraint = [&Offset, &Weights, Locked](double Kappa)
			{
				double Sum = 0.0;
				for (int32 Axis = 0; Axis < Offset.Num(); ++Axis)
				{
					if (IsFree(Locked, Axis))
					{
						Sum += Weights[Axis] * FMath::Square(Offset[Axis] / (1.0 + Kappa * Weights[Axis]));
					}
				}
				return Sum;
			};

			double Lo = 0.0;
			double Hi = 0.0;
			if (Target < FreeQ)
			{
				// Inward: κ > 0, and the constraint falls to 0 as κ grows.
				Hi = 1.0;
				while (Constraint(Hi) > Target && Hi < 1e15)
				{
					Hi *= 4.0;
				}
			}
			else
			{
				Lo = -1.0 / MaxWeight * (1.0 - 1e-12);
				if (Constraint(Lo) < Target)
				{
					// Singular: the most-weighted axes carry no offset, so they take up the rest at κ = -1/w_max.
					const double Kappa = -1.0 / MaxWeight;
					double Used = 0.0;
					for (int32 Axis = 0; Axis < Location.Num(); ++Axis)
					{
						if (!IsFree(Locked, Axis))
						{
							continue;
						}
						if (Weights[Axis] < MaxWeight * (1.0 - 1e-9))
						{
							Location[Axis] = Center[Axis] + Offset[Axis] / (1.0 + Kappa * Weights[Axis]);
							Used += Weights[Axis] * FMath::Square(Location[Axis] - Center[Axis]);
						}
						else
						{
							Location[Axis] = Center[Axis];
						}
					}
					Location[MaxAxis] = Center[MaxAxis] + Sign * FMath::Sqrt(FMath::Max(0.0, Target - Used) / MaxWeight);
					return bReachable;
				}
			}
			for (int32 Step = 0; Step < BisectSteps; ++Step)
			{
				const double Mid = 0.5 * (Lo + Hi);
				// The constraint falls as κ rises.
				(Constraint(Mid) > Target ? Lo : Hi) = Mid;
			}
			const double Kappa = 0.5 * (Lo + Hi);
			for (int32 Axis = 0; Axis < Location.Num(); ++Axis)
			{
				if (IsFree(Locked, Axis))
				{
					Location[Axis] = Center[Axis] + Offset[Axis] / (1.0 + Kappa * Weights[Axis]);
				}
			}
			return bReachable;
		}

		/* Σ_i w_i (l_i - c_i)² = Level; an empty Weights means every weight is 1 (a sphere). */
		struct FQuadric
		{
			TConstArrayView<double> Center;
			TConstArrayView<double> Weights;
			double Level = 0.0;

			double Weight(int32 Axis) const { return Weights.IsEmpty() ? 1.0 : Weights[Axis]; }

			double Value(TConstArrayView<double> Location) const
			{
				double Sum = 0.0;
				for (int32 Axis = 0; Axis < Location.Num(); ++Axis)
				{
					Sum += Weight(Axis) * FMath::Square(Location[Axis] - Center[Axis]);
				}
				return Sum;
			}

			/* The residual scaled to the quadric's own size, so one tolerance serves both. */
			double Residual(TConstArrayView<double> Location) const { return (Value(Location) - Level) / (1.0 + FMath::Abs(Level)); }
		};

		/*
		 * Min-norm Newton steps onto {A = 0, B = 0} from Location, free axes only. Where the two gradients line up (the card
		 * on the line through both centres, say) the system is singular and every step stays on that line, so it first
		 * steps off it along a free perpendicular. False when it doesn't converge; that proves nothing about feasibility.
		 */
		bool NewtonOntoBoth(const FQuadric& A, const FQuadric& B, TConstArrayView<bool> Locked, TArray<double>& Location)
		{
			const int32 Axes = Location.Num();
			double Size = 1.0;
			for (int32 Axis = 0; Axis < Axes; ++Axis)
			{
				Size = FMath::Max(Size, FMath::Abs(A.Center[Axis] - B.Center[Axis]));
			}
			int32 Nudges = 0;
			for (int32 Iteration = 0; Iteration < 200; ++Iteration)
			{
				const double Ra = A.Residual(Location);
				const double Rb = B.Residual(Location);
				if (FMath::Abs(Ra) < 1e-12 && FMath::Abs(Rb) < 1e-12)
				{
					return true;
				}

				TArray<double> Ga;
				TArray<double> Gb;
				Ga.Init(0.0, Axes);
				Gb.Init(0.0, Axes);
				for (int32 Axis = 0; Axis < Axes; ++Axis)
				{
					if (IsFree(Locked, Axis))
					{
						Ga[Axis] = 2.0 * A.Weight(Axis) * (Location[Axis] - A.Center[Axis]) / (1.0 + FMath::Abs(A.Level));
						Gb[Axis] = 2.0 * B.Weight(Axis) * (Location[Axis] - B.Center[Axis]) / (1.0 + FMath::Abs(B.Level));
					}
				}
				double M11 = 0.0;
				double M12 = 0.0;
				double M22 = 0.0;
				for (int32 Axis = 0; Axis < Axes; ++Axis)
				{
					M11 += Ga[Axis] * Ga[Axis];
					M12 += Ga[Axis] * Gb[Axis];
					M22 += Gb[Axis] * Gb[Axis];
				}
				const double Det = M11 * M22 - M12 * M12;
				if (Det <= 1e-9 * M11 * M22 + 1e-300)
				{
					TArray<double> Off;
					const TArray<double> Span[] = {Ga, Gb};
					if (++Nudges > 8 || !FreePerpendicular(Span, Locked, Axes, Off))
					{
						return false;
					}
					for (int32 Axis = 0; Axis < Axes; ++Axis)
					{
						Location[Axis] += 0.05 * Size * Off[Axis];
					}
					continue;
				}

				const double Y1 = (M22 * Ra - M12 * Rb) / Det;
				const double Y2 = (M11 * Rb - M12 * Ra) / Det;
				const double Before = Ra * Ra + Rb * Rb;
				bool bAccepted = false;
				for (double Step = 1.0; Step > 1e-9; Step *= 0.5)
				{
					TArray<double> Trial = Location;
					for (int32 Axis = 0; Axis < Axes; ++Axis)
					{
						Trial[Axis] -= Step * (Ga[Axis] * Y1 + Gb[Axis] * Y2);
					}
					const double TrialA = A.Residual(Trial);
					const double TrialB = B.Residual(Trial);
					if (TrialA * TrialA + TrialB * TrialB < Before)
					{
						Location = MoveTemp(Trial);
						bAccepted = true;
						break;
					}
				}
				if (!bAccepted)
				{
					return false;
				}
			}
			return false;
		}

		/* Where the speaker distance runs least and most over an ellipsoid, and whether it runs without bound. */
		struct FDistanceRange
		{
			bool bAny = false;
			bool bUnbounded = false;
			double Min = SolveInfinity;
			double Max = -SolveInfinity;
			TArray<double> MinAt;
			TArray<double> MaxAt;
		};

		/*
		 * The least and greatest of ||l - P||² over E = {Σ w_i (l_i - M_i)² = Q}, moving only Base's free axes. Every extreme of
		 * a smooth function on E is a stationary point of the Lagrangian: l_i = M_i + d_i / (1 + κ w_i), d = P - M, where
		 * h(κ) = Σ w_i d_i² / (1 + κ w_i)² = Q. h is convex between its poles -1/w_i, so each interval holds at most two roots,
		 * found by bisection either side of its minimum. Axes with no offset add the stationary points at κ = -1/w_j itself.
		 * Exact, so "the range doesn't contain the target" is a proof of infeasibility.
		 */
		FDistanceRange DistanceRangeOnEllipsoid(TConstArrayView<double> P, TConstArrayView<double> M, TConstArrayView<double> W, double Q, TConstArrayView<bool> Locked,
			TConstArrayView<double> Base)
		{
			FDistanceRange Out;
			const int32 Axes = Base.Num();
			double LockedQ = 0.0;
			TArray<int32> Free;
			for (int32 Axis = 0; Axis < Axes; ++Axis)
			{
				if (IsFree(Locked, Axis))
				{
					Free.Add(Axis);
				}
				else
				{
					LockedQ += W[Axis] * FMath::Square(Base[Axis] - M[Axis]);
				}
			}
			const double FreeLevel = Q - LockedQ;
			if (FreeLevel < -1e-12 || Free.IsEmpty())
			{
				return Out;
			}
			const double Level = FMath::Max(0.0, FreeLevel);

			auto Offset = [&P, &M](int32 Axis) { return P[Axis] - M[Axis]; };
			auto Consider = [&](const TArray<double>& Location)
			{
				const double D = SqDistance(Location, P);
				Out.bAny = true;
				if (D < Out.Min)
				{
					Out.Min = D;
					Out.MinAt = Location;
				}
				if (D > Out.Max)
				{
					Out.Max = D;
					Out.MaxAt = Location;
				}
			};
			auto At = [&](double Kappa)
			{
				TArray<double> Location(Base);
				for (const int32 Axis : Free)
				{
					Location[Axis] = M[Axis] + Offset(Axis) / (1.0 + Kappa * W[Axis]);
				}
				return Location;
			};
			auto H = [&](double Kappa)
			{
				double Sum = 0.0;
				for (const int32 Axis : Free)
				{
					Sum += W[Axis] * FMath::Square(Offset(Axis) / (1.0 + Kappa * W[Axis]));
				}
				return Sum;
			};

			for (const int32 Axis : Free)
			{
				// An axis the ellipsoid doesn't weigh lets the card run off along it for ever.
				Out.bUnbounded |= W[Axis] <= SolveTiny;
			}

			TArray<double> Poles;
			for (const int32 Axis : Free)
			{
				if (W[Axis] > SolveTiny && FMath::Abs(Offset(Axis)) > SolveTiny)
				{
					Poles.Add(-1.0 / W[Axis]);
				}
			}
			Poles.Sort();
			TArray<double> Bounds = {-1e15};
			for (const double Pole : Poles)
			{
				if (FMath::Abs(Pole - Bounds.Last()) > 1e-12 * FMath::Max(1.0, FMath::Abs(Pole)))
				{
					Bounds.Add(Pole);
				}
			}
			Bounds.Add(1e15);

			auto Root = [&](double Lo, double Hi, bool bFalling)
			{
				for (int32 Step = 0; Step < 200; ++Step)
				{
					const double Mid = 0.5 * (Lo + Hi);
					const bool bAbove = H(Mid) > Level;
					// Falling: above the level means the root lies further on.
					((bAbove == bFalling) ? Lo : Hi) = Mid;
				}
				return 0.5 * (Lo + Hi);
			};

			for (int32 Interval = 0; Interval + 1 < Bounds.Num(); ++Interval)
			{
				const bool bOpenLeft = Interval == 0;
				const bool bOpenRight = Interval + 2 == Bounds.Num();
				const double Lo = bOpenLeft ? Bounds[Interval] : Bounds[Interval] + 1e-12 * FMath::Max(1.0, FMath::Abs(Bounds[Interval]));
				const double Hi = bOpenRight ? Bounds[Interval + 1] : Bounds[Interval + 1] - 1e-12 * FMath::Max(1.0, FMath::Abs(Bounds[Interval + 1]));
				if (Hi <= Lo)
				{
					continue;
				}

				// h is convex here: golden-section for its least value.
				double A = Lo;
				double B = Hi;
				constexpr double Golden = 0.6180339887498949;
				for (int32 Step = 0; Step < 300; ++Step)
				{
					const double C = B - Golden * (B - A);
					const double D = A + Golden * (B - A);
					(H(C) < H(D) ? B : A) = (H(C) < H(D) ? D : C);
				}
				const double Least = 0.5 * (A + B);
				if (H(Least) > Level)
				{
					continue;
				}
				if (H(Lo) >= Level)
				{
					Consider(At(Root(Lo, Least, true)));
				}
				if (H(Hi) >= Level)
				{
					Consider(At(Root(Least, Hi, false)));
				}
				if (FMath::Abs(H(Least) - Level) <= 1e-12 * (1.0 + Level))
				{
					Consider(At(Least));
				}
			}

			// κ = -1/w_j for an axis with no offset: that axis is free to take up whatever the others leave.
			for (const int32 Axis : Free)
			{
				if (W[Axis] <= SolveTiny || FMath::Abs(Offset(Axis)) > SolveTiny)
				{
					continue;
				}
				const double Kappa = -1.0 / W[Axis];
				auto SameWeight = [&W, Axis](int32 Other) { return FMath::Abs(W[Other] - W[Axis]) <= 1e-9 * W[Axis]; };
				if (Free.ContainsByPredicate([&](int32 Other) { return SameWeight(Other) && FMath::Abs(Offset(Other)) > SolveTiny; }))
				{
					continue;
				}
				TArray<double> Location(Base);
				double Used = 0.0;
				for (const int32 Other : Free)
				{
					if (SameWeight(Other))
					{
						Location[Other] = M[Other];
					}
					else
					{
						Location[Other] = M[Other] + Offset(Other) / (1.0 + Kappa * W[Other]);
						Used += W[Other] * FMath::Square(Location[Other] - M[Other]);
					}
				}
				const double Rest = Level - Used;
				if (Rest < -1e-12 * (1.0 + Level))
				{
					continue;
				}
				const double Reach = FMath::Sqrt(FMath::Max(0.0, Rest) / W[Axis]);
				Location[Axis] = M[Axis] + Reach;
				Consider(Location);
				Location[Axis] = M[Axis] - Reach;
				Consider(Location);
			}
			return Out;
		}

		/*
		 * A point on E at squared distance SqRadius from P, by walking E from its nearest point to its farthest: the distance
		 * runs continuously between them, so it passes through every value in the range. Steps are radial projections onto
		 * E from its centre; a half-turn is split through a perpendicular so the path never passes through the centre.
		 */
		bool WalkEllipsoid(TConstArrayView<double> P, double SqRadius, TConstArrayView<double> M, TConstArrayView<double> W, double Q, TConstArrayView<bool> Locked,
			TConstArrayView<double> Base, const FDistanceRange& Range, TArray<double>& Out)
		{
			const int32 Axes = Base.Num();
			double LockedQ = 0.0;
			for (int32 Axis = 0; Axis < Axes; ++Axis)
			{
				LockedQ += IsFree(Locked, Axis) ? 0.0 : W[Axis] * FMath::Square(Base[Axis] - M[Axis]);
			}
			const double Level = FMath::Max(0.0, Q - LockedQ);
			auto OnEllipsoid = [&](const TArray<double>& V, TArray<double>& Location)
			{
				double Weighted = 0.0;
				for (int32 Axis = 0; Axis < Axes; ++Axis)
				{
					Weighted += IsFree(Locked, Axis) ? W[Axis] * V[Axis] * V[Axis] : 0.0;
				}
				if (Weighted <= 1e-300)
				{
					return false;
				}
				const double Scale = FMath::Sqrt(Level / Weighted);
				Location = TArray<double>(Base);
				for (int32 Axis = 0; Axis < Axes; ++Axis)
				{
					if (IsFree(Locked, Axis))
					{
						Location[Axis] = M[Axis] + Scale * V[Axis];
					}
				}
				return true;
			};
			auto FreeOffset = [&](TConstArrayView<double> Location)
			{
				TArray<double> V;
				V.Init(0.0, Axes);
				for (int32 Axis = 0; Axis < Axes; ++Axis)
				{
					V[Axis] = IsFree(Locked, Axis) ? Location[Axis] - M[Axis] : 0.0;
				}
				return V;
			};

			if (Range.bUnbounded)
			{
				// Run off along an axis E doesn't weigh, away from P, until far enough.
				int32 Run = INDEX_NONE;
				for (int32 Axis = 0; Axis < Axes && Run == INDEX_NONE; ++Axis)
				{
					Run = IsFree(Locked, Axis) && W[Axis] <= SolveTiny ? Axis : INDEX_NONE;
				}
				if (Run == INDEX_NONE)
				{
					return false;
				}
				const double Away = Range.MinAt[Run] >= P[Run] ? 1.0 : -1.0;
				auto D = [&](double T)
				{
					TArray<double> Location(Range.MinAt);
					Location[Run] += Away * T;
					return SqDistance(Location, P);
				};
				double Hi = 1.0;
				while (D(Hi) < SqRadius && Hi < 1e15)
				{
					Hi *= 2.0;
				}
				double Lo = 0.0;
				for (int32 Step = 0; Step < BisectSteps; ++Step)
				{
					const double Mid = 0.5 * (Lo + Hi);
					(D(Mid) < SqRadius ? Lo : Hi) = Mid;
				}
				Out = Range.MinAt;
				Out[Run] += Away * 0.5 * (Lo + Hi);
				return true;
			}

			const TArray<double> From = FreeOffset(Range.MinAt);
			const TArray<double> To = FreeOffset(Range.MaxAt);
			double Dot = 0.0;
			double FromSq = 0.0;
			double ToSq = 0.0;
			for (int32 Axis = 0; Axis < Axes; ++Axis)
			{
				Dot += From[Axis] * To[Axis];
				FromSq += From[Axis] * From[Axis];
				ToSq += To[Axis] * To[Axis];
			}
			TArray<TPair<TArray<double>, TArray<double>>> Legs;
			if (Dot < -(1.0 - 1e-9) * FMath::Sqrt(FromSq * ToSq))
			{
				TArray<double> Side;
				const TArray<double> Span[] = {From};
				if (!FreePerpendicular(Span, Locked, Axes, Side))
				{
					return false;
				}
				for (double& Value : Side)
				{
					Value *= FMath::Sqrt(FromSq);
				}
				Legs.Add({From, Side});
				Legs.Add({Side, To});
			}
			else
			{
				Legs.Add({From, To});
			}

			for (const TPair<TArray<double>, TArray<double>>& Leg : Legs)
			{
				auto F = [&](double S, TArray<double>& Location)
				{
					TArray<double> V;
					V.SetNum(Axes);
					for (int32 Axis = 0; Axis < Axes; ++Axis)
					{
						V[Axis] = (1.0 - S) * Leg.Key[Axis] + S * Leg.Value[Axis];
					}
					return OnEllipsoid(V, Location) ? SqDistance(Location, P) - SqRadius : SolveInfinity;
				};
				constexpr int32 Samples = 64;
				TArray<double> Location;
				double Prev = F(0.0, Location);
				if (FMath::Abs(Prev) <= 1e-12 * (1.0 + SqRadius))
				{
					Out = Location;
					return true;
				}
				for (int32 Sample = 1; Sample <= Samples; ++Sample)
				{
					const double S = static_cast<double>(Sample) / Samples;
					const double Next = F(S, Location);
					if (Prev != SolveInfinity && Next != SolveInfinity && (Prev <= 0.0) != (Next <= 0.0))
					{
						double Lo = static_cast<double>(Sample - 1) / Samples;
						double Hi = S;
						const bool bRising = Prev <= 0.0;
						for (int32 Step = 0; Step < BisectSteps; ++Step)
						{
							const double Mid = 0.5 * (Lo + Hi);
							((F(Mid, Location) <= 0.0) == bRising ? Lo : Hi) = Mid;
						}
						F(0.5 * (Lo + Hi), Out);
						return true;
					}
					Prev = Next;
				}
			}
			return false;
		}

		/*
		 * P(g) and P(y) for one card moved, without re-running the catalog: every other family's weight and every other
		 * wording's utility are read from the original evaluation, and only the moved card's family is rescored, by the
		 * same rule Evaluate uses (legal wordings in reach, or all legal ones when none is; illegal families take no share).
		 */
		struct FProbe
		{
			const FParams& Params;
			const FPhrasing& Card;
			double Affinity = 0.0;
			double Beta = 1.0;
			bool bCardLegal = true;
			bool bFamilyIllegal = false;
			TArray<double> OtherLogits;
			TArray<bool> OtherInReach;
			/* ln(e^(β u_∅) + Σ w̄_g') over every other family in the partition. */
			double RestLogZ = 0.0;

			FProbe(const FParams& InParams, TConstArrayView<FPhrasing> Phrasings, double InAffinity, int32 Index, const FEvaluation& Original)
				: Params(InParams), Card(Phrasings[Index]), Affinity(InAffinity)
			{
				Beta = FMath::Max(Params.Beta, UE_DOUBLE_SMALL_NUMBER);
				bCardLegal = Card.IsLegal();
				for (int32 Other = 0; Other < Phrasings.Num(); ++Other)
				{
					if (Other != Index && Phrasings[Other].Family == Card.Family && Phrasings[Other].IsLegal())
					{
						OtherLogits.Add(Beta * Original.Phrasings[Other].Utility);
						OtherInReach.Add(Original.Phrasings[Other].SpeakerReach >= Params.ReachFloor);
					}
				}
				bFamilyIllegal = !bCardLegal && OtherLogits.IsEmpty();

				TArray<double> Rest = {Original.SilenceLogWeight};
				for (const FFamilyScore& Family : Original.Families)
				{
					if (Family.Family != Card.Family && Family.Eligibility != EFamilyEligibility::Illegal)
					{
						Rest.Add(Family.LogMeanWeight);
					}
				}
				RestLogZ = SolveLogSumExp(Rest);
			}

			void Measure(TConstArrayView<double> Location, double& OutFamily, double& OutCard) const
			{
				OutFamily = 0.0;
				OutCard = 0.0;
				if (bFamilyIllegal)
				{
					return;
				}
				const double Resonance = ResonanceAt(Card, Location);
				const bool bCardInReach = FMath::Exp(Resonance) >= Params.ReachFloor;
				const double Logit = Beta * (Resonance + RelevanceAt(Params, Card, Location) + Params.Lambda * Card.Stance * Affinity + Card.UtilityBias);
				const bool bAnyInReach = (bCardLegal && bCardInReach) || OtherInReach.Contains(true);

				TArray<double, TInlineAllocator<16>> Scored;
				const bool bCardScored = bCardLegal && (bCardInReach || !bAnyInReach);
				if (bCardScored)
				{
					Scored.Add(Logit);
				}
				for (int32 Other = 0; Other < OtherLogits.Num(); ++Other)
				{
					if (OtherInReach[Other] || !bAnyInReach)
					{
						Scored.Add(OtherLogits[Other]);
					}
				}
				const double LogSum = SolveLogSumExp(Scored);
				const double LogMean = LogSum - FMath::Loge(static_cast<double>(Scored.Num()));
				const double LogZ = SolveLogSumExp({RestLogZ, LogMean});
				OutFamily = FMath::Exp(LogMean - LogZ);
				OutCard = bCardScored ? OutFamily * FMath::Exp(Logit - LogSum) : 0.0;
			}
		};
	}
	using namespace SolveDetail;

	const TCHAR* LexToString(ESolveTarget Target)
	{
		switch (Target)
		{
		case ESolveTarget::Relevance: return TEXT("relevance");
		case ESolveTarget::Utility: return TEXT("utility");
		case ESolveTarget::CardProbability: return TEXT("P(y)");
		case ESolveTarget::FamilyProbability: return TEXT("P(g)");
		default: return TEXT("resonance");
		}
	}

	double MeasureTarget(ESolveTarget Target, const FEvaluation& Evaluation, int32 Phrasing)
	{
		if (!Evaluation.Phrasings.IsValidIndex(Phrasing))
		{
			return 0.0;
		}
		const FPhrasingTerms& Terms = Evaluation.Phrasings[Phrasing];
		switch (Target)
		{
		case ESolveTarget::Relevance: return Terms.Relevance;
		case ESolveTarget::Utility: return Terms.Utility;
		case ESolveTarget::CardProbability: return Terms.Probability;
		case ESolveTarget::FamilyProbability:
			for (const FFamilyScore& Family : Evaluation.Families)
			{
				if (Family.Phrasings.Contains(Phrasing))
				{
					return Family.Probability;
				}
			}
			return 0.0;
		default: return Terms.Resonance;
		}
	}

	TArray<double> MaxUtilityLocation(const FParams& Params, const FPhrasing& Phrasing, TConstArrayView<bool> LockedAxes)
	{
		TArray<double> Out = Phrasing.Location;
		const TArray<double> Weights = RelevanceWeights(Params, Phrasing);
		const double Pull = 1.0 / SolveSigma2(Phrasing);
		for (int32 Axis = 0; Axis < Out.Num(); ++Axis)
		{
			if (IsFree(LockedAxes, Axis))
			{
				// d/dl of -(x - l)²/σ² - w (μ - l)² vanishes at the precision-weighted mean of x and μ.
				Out[Axis] = (Pull * Phrasing.SpeakerX[Axis] + Weights[Axis] * Phrasing.BeliefMean[Axis]) / (Pull + Weights[Axis]);
			}
		}
		return Out;
	}

	bool MaxUtilityLocationInReach(const FParams& Params, const FPhrasing& Phrasing, TConstArrayView<bool> LockedAxes, TArray<double>& OutLocation)
	{
		OutLocation = MaxUtilityLocation(Params, Phrasing, LockedAxes);
		const double ReachSq = ReachSqRadius(Params, Phrasing);
		if (ReachSq < 0.0 || SqDistance(OutLocation, Phrasing.SpeakerX) <= ReachSq)
		{
			return true;
		}
		double LockedSq = 0.0;
		for (int32 Axis = 0; Axis < OutLocation.Num(); ++Axis)
		{
			LockedSq += IsFree(LockedAxes, Axis) ? 0.0 : FMath::Square(Phrasing.Location[Axis] - Phrasing.SpeakerX[Axis]);
		}
		if (LockedSq > ReachSq)
		{
			return false;
		}

		// The constrained peak: the resonance pull stiffened by ν ≥ 0 until the card sits on the reach sphere.
		const TArray<double> Weights = RelevanceWeights(Params, Phrasing);
		const double Pull = 1.0 / SolveSigma2(Phrasing);
		auto At = [&](double Nu)
		{
			TArray<double> Location = Phrasing.Location;
			for (int32 Axis = 0; Axis < Location.Num(); ++Axis)
			{
				if (IsFree(LockedAxes, Axis))
				{
					Location[Axis] = ((Pull + Nu) * Phrasing.SpeakerX[Axis] + Weights[Axis] * Phrasing.BeliefMean[Axis]) / (Pull + Nu + Weights[Axis]);
				}
			}
			return Location;
		};
		double Lo = 0.0;
		double Hi = 1.0;
		while (SqDistance(At(Hi), Phrasing.SpeakerX) > ReachSq && Hi < 1e15)
		{
			Hi *= 4.0;
		}
		for (int32 Step = 0; Step < 100; ++Step)
		{
			const double Mid = 0.5 * (Lo + Hi);
			(SqDistance(At(Mid), Phrasing.SpeakerX) > ReachSq ? Lo : Hi) = Mid;
		}
		OutLocation = At(Hi);
		return true;
	}

	FEvaluation EvaluateMoved(const FParams& Params, TConstArrayView<FPhrasing> Phrasings, double Affinity, int32 Phrasing, TConstArrayView<double> Location)
	{
		TArray<FPhrasing> Moved(Phrasings);
		if (Moved.IsValidIndex(Phrasing) && Location.Num() == Moved[Phrasing].Location.Num())
		{
			Moved[Phrasing].Location = TArray<double>(Location);
		}
		return Evaluate(Params, Moved, Affinity);
	}

	FSolveResult SolveLocation(const FParams& Params, TConstArrayView<FPhrasing> Phrasings, double Affinity, const FSolveRequest& Request)
	{
		FSolveResult Out;
		if (!Phrasings.IsValidIndex(Request.Phrasing))
		{
			Out.Note = TEXT("No card selected.");
			return Out;
		}

		const FPhrasing& Card = Phrasings[Request.Phrasing];
		const int32 Axes = Card.Location.Num();
		Out.Location = Card.Location;
		const FEvaluation Original = Evaluate(Params, Phrasings, Affinity);
		Out.Before = MeasureTarget(Request.Target, Original, Request.Phrasing);

		// One full evaluation at the end, for the preview and the verdict.
		auto Finish = [&]()
		{
			Out.Evaluation = EvaluateMoved(Params, Phrasings, Affinity, Request.Phrasing, Out.Location);
			Out.Achieved = MeasureTarget(Request.Target, Out.Evaluation, Request.Phrasing);
			const double Tolerance = IsProbability(Request.Target) ? ProbabilityTolerance : TermTolerance;
			Out.bSolved = Out.Note.IsEmpty() && FMath::Abs(Out.Achieved - Request.Value) <= Tolerance;
			if (!Out.bSolved && Out.Note.IsEmpty())
			{
				Out.Note = FString::Printf(TEXT("Closest reachable is %s %s."), LexToString(Request.Target), *FormatValue(Request.Target, Out.Achieved));
			}
			return Out;
		};

		if (Axes == 0 || Card.SpeakerX.Num() != Axes || Card.BeliefMean.Num() != Axes || Card.BeliefVariance.Num() != Axes)
		{
			Out.Note = TEXT("The card has no mask axes to move.");
			return Finish();
		}
		if (CountFree(Request.LockedAxes, Axes) == 0)
		{
			Out.Note = TEXT("Every axis is locked: unlock one to let the card move.");
			return Finish();
		}

		const TArray<double> Weights = RelevanceWeights(Params, Card);
		const bool bRelevanceMoves = Weights.ContainsByPredicate([](double W) { return W > UE_DOUBLE_SMALL_NUMBER; });
		// Off-centre fallbacks lean toward the other term's centre, so the move costs that term the least.
		const TArray<double> TowardBelief = Difference(Card.BeliefMean, Card.SpeakerX);
		const TArray<double> TowardSpeaker = Difference(Card.SpeakerX, Card.BeliefMean);

		if (Request.Target == ESolveTarget::Resonance || Request.Target == ESolveTarget::Relevance)
		{
			const bool bResonance = Request.Target == ESolveTarget::Resonance;
			double Value = Request.Value;
			if (bResonance && Value > 0.0)
			{
				Out.Note = TEXT("Resonance is -d^2/s^2, so it can't exceed 0 (the card on the speaker).");
				Value = 0.0;
			}
			const double Ceiling = RelevanceCeiling(Params, Card);
			if (!bResonance && Value > Ceiling)
			{
				Out.Note = FString::Printf(TEXT("Relevance can't exceed %.3f here (the card on the belief, under its spread)."), Ceiling);
				Value = Ceiling;
			}
			if (!bResonance && !bRelevanceMoves)
			{
				Out.Note = TEXT("Relevance doesn't depend on location here (eta is 0).");
				return Finish();
			}

			const double SphereLevel = bResonance ? ResonanceSqRadius(Card, Value) : ResonanceSqRadius(Card, ResonanceAt(Card, Card.Location));
			const double EllipsoidLevel = bResonance ? RelevanceQ(Params, Card, RelevanceAt(Params, Card, Card.Location)) : RelevanceQ(Params, Card, Value);

			auto ProjectTarget = [&]()
			{
				return bResonance
					? ProjectSphere(Card.SpeakerX, Out.Location, SphereLevel, Request.LockedAxes, TowardBelief)
					: ProjectEllipsoid(Card.BeliefMean, Weights, Out.Location, EllipsoidLevel, Request.LockedAxes, TowardSpeaker);
			};

			// Holding relevance when it can't move holds itself.
			if (!Request.bHoldOther || (bResonance && !bRelevanceMoves))
			{
				if (!ProjectTarget() && Out.Note.IsEmpty())
				{
					Out.Note = TEXT("The locked axes alone already put the card past that value.");
				}
				return Finish();
			}

			const FQuadric Sphere{Card.SpeakerX, {}, SphereLevel};
			const FQuadric Ellipsoid{Card.BeliefMean, Weights, EllipsoidLevel};
			const TCHAR* Held = bResonance ? TEXT("relevance") : TEXT("resonance");
			const double HeldValue = bResonance ? RelevanceAt(Params, Card, Card.Location) : ResonanceAt(Card, Card.Location);

			// 1. Newton from the card: lands near it when it converges.
			TArray<double> Location = Card.Location;
			if (NewtonOntoBoth(Sphere, Ellipsoid, Request.LockedAxes, Location))
			{
				Out.Location = MoveTemp(Location);
				return Finish();
			}

			// 2. The exact range of speaker distance over the ellipsoid says whether the two meet at all.
			const FDistanceRange Range = DistanceRangeOnEllipsoid(Card.SpeakerX, Card.BeliefMean, Weights, EllipsoidLevel, Request.LockedAxes, Card.Location);
			if (!Range.bAny)
			{
				if (Out.Note.IsEmpty())
				{
					Out.Note = TEXT("The locked axes alone already put relevance past that level: unlock an axis.");
				}
				ProjectTarget();
				return Finish();
			}
			const double Slack = 1e-9 * (1.0 + SphereLevel);
			const bool bMeet = Range.Min <= SphereLevel + Slack && (Range.bUnbounded || SphereLevel <= Range.Max + Slack);
			if (bMeet)
			{
				if (WalkEllipsoid(Card.SpeakerX, SphereLevel, Card.BeliefMean, Weights, EllipsoidLevel, Request.LockedAxes, Card.Location, Range, Location))
				{
					// Polish from the walk's point, then keep it.
					TArray<double> Polished = Location;
					Out.Location = NewtonOntoBoth(Sphere, Ellipsoid, Request.LockedAxes, Polished) ? MoveTemp(Polished) : MoveTemp(Location);
					return Finish();
				}
				Out.Note = FString::Printf(TEXT("A point holding %s at %.3f exists, but the search didn't converge on it. Try unlocking an axis."), Held, HeldValue);
			}
			else
			{
				// Proven: with relevance at its level, resonance can only run between these.
				const double Sigma2 = SolveSigma2(Card);
				const double ResonanceHigh = -Range.Min / Sigma2;
				const FString Low = Range.bUnbounded ? FString(TEXT("-inf")) : FString::Printf(TEXT("%.3f"), -Range.Max / Sigma2);
				Out.Note = bResonance
					? FString::Printf(TEXT("Can't hold relevance at %.3f as well: there, resonance can only run from %s to %.3f on the free axes. Met the target; relevance moved."),
						HeldValue, *Low, ResonanceHigh)
					: FString::Printf(TEXT("Can't hold resonance at %.3f as well: at relevance %.3f, resonance can only run from %s to %.3f on the free axes. Met the target; resonance moved."),
						HeldValue, Value, *Low, ResonanceHigh);
			}

			// The compromise: the target met exactly, the held term as near as it gets (the extreme nearest it).
			const bool bBelow = SphereLevel < Range.Min;
			Location = (bBelow || Range.bUnbounded) ? Range.MinAt : Range.MaxAt;
			if (bResonance)
			{
				ProjectSphere(Card.SpeakerX, Location, SphereLevel, Request.LockedAxes, TowardBelief);
			}
			Out.Location = MoveTemp(Location);
			return Finish();
		}

		// Utility and the chances: searched on lines through the card.
		if (IsProbability(Request.Target))
		{
			if (Request.Value <= 0.0 || Request.Value >= 1.0)
			{
				Out.Note = TEXT("A probability target must sit strictly between 0% and 100%.");
				return Finish();
			}
			if (!Card.IsLegal())
			{
				Out.Note = FString::Printf(TEXT("This card isn't legal for the speaker (check %d), so it is never scored: moving it can't change a chance."), Card.IllegalCheck);
				return Finish();
			}
		}
		const FProbe Probe(Params, Phrasings, Affinity, Request.Phrasing, Original);
		auto Measure = [&](TConstArrayView<double> Location)
		{
			if (Request.Target == ESolveTarget::Utility)
			{
				return UtilityAt(Params, Card, Affinity, Location);
			}
			double Family = 0.0;
			double Own = 0.0;
			Probe.Measure(Location, Family, Own);
			return Request.Target == ESolveTarget::FamilyProbability ? Family : Own;
		};

		struct FLine
		{
			TArray<double> Dir;
		};
		TArray<FLine> Lines;
		auto AddLine = [&](TConstArrayView<double> Toward)
		{
			TArray<double> Dir = Difference(Toward, Card.Location);
			double Length = 0.0;
			for (const double Value : Dir)
			{
				Length += Value * Value;
			}
			if (Length <= UE_DOUBLE_SMALL_NUMBER)
			{
				Dir = FreeDirection(TowardBelief, Request.LockedAxes, Axes);
			}
			Lines.Add({MoveTemp(Dir)});
		};
		// Toward the peak, and toward the best spot still in reach when the peak lies beyond it.
		const TArray<double> Peak = MaxUtilityLocation(Params, Card, Request.LockedAxes);
		AddLine(Peak);
		TArray<double> BestInReach;
		if (MaxUtilityLocationInReach(Params, Card, Request.LockedAxes, BestInReach) && SqDistance(BestInReach, Peak) > 1e-18)
		{
			AddLine(BestInReach);
		}

		const double ReachSq = ReachSqRadius(Params, Card);
		double Spread = 0.0;
		for (const double Variance : Card.BeliefVariance)
		{
			Spread = FMath::Max(Spread, FMath::Max(0.0, Variance));
		}
		// Far enough out that every term has flattened: past the speaker and the belief by many reaches.
		const double Far = FMath::Sqrt(SqDistance(Card.Location, Card.SpeakerX)) + FMath::Sqrt(SqDistance(Card.Location, Card.BeliefMean))
			+ 12.0 * FMath::Sqrt(SolveSigma2(Card)) + 12.0 * FMath::Sqrt(Spread) + 10.0;

		struct FCandidate
		{
			bool bSet = false;
			double Value = 0.0;
			double Move = SolveInfinity;
			bool bAtReachEdge = false;
			TArray<double> Location;
		};
		FCandidate Hit;
		FCandidate Closest;

		for (const FLine& Line : Lines)
		{
			double DirSq = 0.0;
			for (const double Value : Line.Dir)
			{
				DirSq += Value * Value;
			}
			const double Span = Far / FMath::Sqrt(DirSq);
			auto At = [&](double T)
			{
				TArray<double> Location = Card.Location;
				for (int32 Axis = 0; Axis < Axes; ++Axis)
				{
					Location[Axis] += T * Line.Dir[Axis];
				}
				return Location;
			};

			// Cuts: the ends, the reach sphere's crossings (a jump), and the utility peak (a turn).
			struct FCut
			{
				double T;
				bool bJump;
			};
			TArray<FCut> Cuts = {{-Span, false}, {Span, false}};
			if (ReachSq >= 0.0)
			{
				double B = 0.0;
				for (int32 Axis = 0; Axis < Axes; ++Axis)
				{
					B += 2.0 * Line.Dir[Axis] * (Card.Location[Axis] - Card.SpeakerX[Axis]);
				}
				const double C = SqDistance(Card.Location, Card.SpeakerX) - ReachSq;
				const double Disc = B * B - 4.0 * DirSq * C;
				if (Disc > 0.0)
				{
					for (const double Sign : {-1.0, 1.0})
					{
						const double T = (-B + Sign * FMath::Sqrt(Disc)) / (2.0 * DirSq);
						if (T > -Span && T < Span)
						{
							Cuts.Add({T, true});
						}
					}
				}
			}
			{
				const double U0 = UtilityAt(Params, Card, Affinity, At(0.0));
				const double U1 = UtilityAt(Params, Card, Affinity, At(1.0));
				const double Um = UtilityAt(Params, Card, Affinity, At(-1.0));
				const double A = 0.5 * (U1 + Um - 2.0 * U0);
				const double B = 0.5 * (U1 - Um);
				if (A < -SolveTiny)
				{
					const double PeakT = -B / (2.0 * A);
					if (PeakT > -Span && PeakT < Span)
					{
						Cuts.Add({PeakT, false});
					}
				}
			}
			Cuts.Sort([](const FCut& L, const FCut& R) { return L.T < R.T; });

			for (int32 Piece = 0; Piece + 1 < Cuts.Num(); ++Piece)
			{
				const FCut& Left = Cuts[Piece];
				const FCut& Right = Cuts[Piece + 1];
				const double Width = Right.T - Left.T;
				if (Width <= 1e-14)
				{
					continue;
				}
				// Just inside a jump, so each piece is read on its own side of it.
				const double TA = Left.T + (Left.bJump ? 1e-7 * Width : 0.0);
				const double TB = Right.T - (Right.bJump ? 1e-7 * Width : 0.0);
				const double MA = Measure(At(TA));
				const double MB = Measure(At(TB));

				// The closest value seen at any piece's end, nearer moves winning ties: the answer when nothing crosses.
				for (const TPair<double, bool>& End : {TPair<double, bool>(TA, Left.bJump), TPair<double, bool>(TB, Right.bJump)})
				{
					const double Value = End.Key == TA ? MA : MB;
					TArray<double> Location = At(End.Key);
					const double Move = FMath::Sqrt(SqDistance(Location, Card.Location));
					const double Gap = FMath::Abs(Value - Request.Value);
					const double Held = Closest.bSet ? FMath::Abs(Closest.Value - Request.Value) : SolveInfinity;
					if (!Closest.bSet || Gap < Held - 1e-15 || (Gap <= Held + 1e-15 && Move < Closest.Move))
					{
						Closest = {true, Value, Move, End.Value, MoveTemp(Location)};
					}
				}

				if ((Request.Value - MA) * (Request.Value - MB) <= 0.0)
				{
					// Monotone between cuts: bisect toward the target.
					double Lo = TA;
					double Hi = TB;
					const bool bRising = MA <= MB;
					for (int32 Step = 0; Step < BisectSteps; ++Step)
					{
						const double Mid = 0.5 * (Lo + Hi);
						((Measure(At(Mid)) < Request.Value) == bRising ? Lo : Hi) = Mid;
					}
					const double T = 0.5 * (Lo + Hi);
					const TArray<double> Location = At(T);
					const double Move = FMath::Sqrt(SqDistance(Location, Card.Location));
					if (!Hit.bSet || Move < Hit.Move)
					{
						Hit = {true, Measure(Location), Move, false, Location};
					}
				}
			}
		}

		if (Hit.bSet)
		{
			Out.Location = MoveTemp(Hit.Location);
			return Finish();
		}

		Out.Location = Closest.Location;
		const bool bAbove = Request.Value > Closest.Value;
		Out.Note = FString::Printf(TEXT("%s %s here"), bAbove ? TEXT("At most") : TEXT("At least"), *FormatValue(Request.Target, Closest.Value));
		if (Closest.bAtReachEdge)
		{
			Out.Note += FString::Printf(TEXT(", at the edge of the speaker's reach (K = %.2f): across it the card %s its family's weight."), Params.ReachFloor,
				bAbove ? TEXT("leaves") : TEXT("would leave"));
		}
		else
		{
			Out.Note += bAbove ? TEXT(": that's the card at its best spot.") : TEXT(".");
		}
		return Finish();
	}
}
