#pragma once

#include "EDXPrerequisites.h"
#include "ForwardDecl.h"
#include "Containers/DimensionalArray.h"
#include "Math/Vector.h"
#include "../pcgsolver/sparse_matrix.h"
#include "../pcgsolver/pcg_solver.h"

#include "../Water/LevelSet.h"

#include <vector>
using namespace std;

namespace EDX
{
	namespace FluidSim
	{
		template<uint Dimension>
		struct Particle
		{
			Vec<Dimension, float> vPos;
			Vec<Dimension, float> vVel;
			Particle(const Vec<Dimension, float>& pos, const Vec<Dimension, float>& vel = Vec<Dimension, float>::ZERO)
				: vPos(pos)
				, vVel(vel)
			{
			}
		};

		template<uint Dimension>
		class FluidSolver
		{
		protected:
			Vec<Dimension, uint> mvDim;
			Vec<Dimension, float> mvExtent;
			float mfDx;
			float mfInvDx;
			bool mbUseFLIP;
			bool mbReseedParticles;
			bool mbRebuildPossionPerFrame;
			bool mbUseVorticityConfinement;
			bool mbAllowPartiallyFilled;

			Vec<Dimension, uint> mavOffsetTable[Math::Pow2<Dimension>::Value];
			Vec<Dimension, int> mavOffsetTable2ndOrder[Math::Pow4<Dimension>::Value];

			uint miFrameIdx;

			// Flip Particles
			vector<Particle<Dimension>> mParticles;
			DimensionalArray<Dimension, float> mWeights;
			DimensionalArray<Dimension, int> mParticleCounts;

			// Fluid data
			DimensionalArray<Dimension, float> mVelocity[Dimension];
			DimensionalArray<Dimension, float> mTempVel[Dimension];
			DimensionalArray<Dimension, float> mDeltaVel[Dimension];
			DimensionalArray<Dimension, float> mMCBuffer;
			DimensionalArray<Dimension, float> mVelocityMCBuffer[Dimension];
			
			DimensionalArray<Dimension, float> mVolume[Dimension];
			DimensionalArray<Dimension, float> mFluidVolume[Dimension];
			

			// Vorticity Confinement buffers
			DimensionalArray<Dimension, Vector3> mCurl;
			DimensionalArray<Dimension, float> mCurlMag;
			DimensionalArray<Dimension, Vector3> mVCForce;

			// PCG Solver data
			PCGSolver<double> mPCGSolver;
			SparseMatrixd mMatrix;
			DimensionalArray<Dimension, double> mDivergence;

		public:
			DimensionalArray<Dimension, CellType> mMarkers;
			LevelSet<Dimension> mLevelSet;
			LevelSet<Dimension> mSolidPhi;
			DimensionalArray<Dimension, double> mPressure;
			// Visual data
			DimensionalArray<Dimension, float> mAvgVelocity[Dimension];
			DimensionalArray<Dimension, float> mVisual;

		public:
			void Initialize(const Vec<Dimension, uint>& vDim, const Vec<Dimension, float> vFluidExtent);
			void Advance(const float fDt);

			Vec<Dimension, uint> GetDimension() const { return mvDim; }
			
		protected:
			virtual void Step(const float fDt);
			// Finite element advection solver
			void AdvectVelocitySemiLag(const float fDt,
				DimensionalArray<Dimension, float> grid[Dimension],
				DimensionalArray<Dimension, float> prevVelGrid[Dimension],
				DimensionalArray<Dimension, float> velGrid[Dimension],
				const bool b2ndOrderLerp = false);
			void AdvectVelocityMacCormack(const float fDt,
				DimensionalArray<Dimension, float> velGrid[Dimension],
				DimensionalArray<Dimension, float> prevVelGrid[Dimension]);
			void AdvectValueSemiLag(const float fDt,
				DimensionalArray<Dimension, float>& grid,
				DimensionalArray<Dimension, float>& prevGrid,
				const bool b2ndOrderLerp = false);
			void AdvectValueMacCormack(const float fDt,
				DimensionalArray<Dimension, float>& grid,
				DimensionalArray<Dimension, float>& prevGrid);
			void DiffuseValueGaussSedel(const float fDt,
				const float fRate,
				DimensionalArray<Dimension, float>& grid,
				DimensionalArray<Dimension, float>& prevGrid);
			void VorticityConfinement(const float fDt,
				const float fRate);
			void CalcVelocityAtCellCenter();
			virtual void AddSources(const float fDt);

			// FLIP/PIC solver
			virtual void SeedParticles();
			void ReseedParticles();
			void AdvectParticles(const float fDt);
			void SaveVelocity();
			void GetVelocityUpdate();
			void TransferParticlesToGrid();
			void TransferGridToParticles();

			void ComputeVolume();
			void SetBoundaryMarkers();
			virtual void UpdateMarkers() {}
			void FormMatrixAndPreconditioner();
			void Project(const float fDt);
			void ConstrainVelocity();
			float GetCFL() const;

			// General MAC grid operation
			Vec<Dimension, float> GetValueFace(const Vec<Dimension, float>& vPos, const DimensionalArray<Dimension, float> velField[Dimension], const bool b2ndOrderLerp = false) const;
			float GetValueFace(const Vec<Dimension, float>& vPos, const DimensionalArray<Dimension, float> velField[Dimension], const Component comp, const bool b2ndOrderLerp = false) const;
			float GetValue(const Vec<Dimension, float>& vPos, const DimensionalArray<Dimension, float>& valField, const bool b2ndOrderLerp = false) const;
			Vec<Dimension, float> GetValue(const Vec<Dimension, float>& vPos, const DimensionalArray<Dimension, float> valField[Dimension], const bool b2ndOrderLerp = false) const;
			float InterpolateValue(const Vec<Dimension, float>& vPos, const DimensionalArray<Dimension, float>& grid) const;
			float InterpolateValue2ndOrder(const Vec<Dimension, float>& vPos, const DimensionalArray<Dimension, float>& grid) const;
			Vec<Dimension, float> InterpolateGradient(const Vec<Dimension, float>& vPos, const DimensionalArray<Dimension, float>& grid) const;
			float FractionInside(const float fPhiLeft, const float fPhiRight) const;

			template<typename PhiFunc>
			void SetMarkers(PhiFunc func);
			
			void WriteToFile() const {}

		public:
			const vector<Particle<Dimension>>& GetParticles() const { return mParticles; }
			void AddVelocitySrc(const Vec<Dimension, int>& vIdx, const Vec<Dimension, float>& vVel);
		};

	}
}