/*
*/

#ifndef LANCZOS_HEISENBERG_H
#define LANCZOS_HEISENBERG_H

#include "CrsMatrix.h"
#include "BitManip.h"
#include "TypeToString.h"
#include "SparseRow.h"
#include "BasisHeisenberg.h"
#include "ParametersHeisenberg.h"
#include "ModelBase.h"

namespace LanczosPlusPlus {

template<typename ComplexOrRealType,typename GeometryType,typename InputType>
class Heisenberg  : public ModelBase<ComplexOrRealType,GeometryType,InputType> {

	typedef typename PsimagLite::Real<ComplexOrRealType>::Type RealType;
	typedef PsimagLite::Matrix<ComplexOrRealType> MatrixType;
	typedef std::pair<SizeType,SizeType> PairType;
	typedef ModelBase<ComplexOrRealType,GeometryType,InputType> BaseType;

	enum {SPIN_UP = ProgramGlobals::SPIN_UP, SPIN_DOWN = ProgramGlobals::SPIN_DOWN};

public:

	typedef ParametersHeisenberg<RealType,InputType> ParametersModelType;
	typedef BasisHeisenberg<GeometryType> BasisType;
	typedef typename BasisType::BaseType BasisBaseType;
	typedef typename BasisType::WordType WordType;
	typedef typename BaseType::SparseMatrixType SparseMatrixType;
	typedef typename BaseType::VectorType VectorType;
	typedef PsimagLite::SparseRow<SparseMatrixType> SparseRowType;

	static int const FERMION_SIGN = BasisType::FERMION_SIGN;

	Heisenberg(SizeType nup,
	       SizeType ndown,
	       InputType& io,
	       const GeometryType& geometry)
	    : mp_(io),
	      geometry_(geometry),
	      basis_(geometry,nup,ndown),
	      jpm_(geometry_.numberOfSites(),geometry_.numberOfSites()),
	      jzz_(geometry_.numberOfSites(),geometry_.numberOfSites())
	{
		SizeType n = geometry_.numberOfSites();

		if (geometry_.terms() != 2) {
			PsimagLite::String msg("Heisenberg: must have either 2 terms\n");
			throw PsimagLite::RuntimeError(msg);
		}

		for (SizeType i=0;i<n;i++) {
			for (SizeType j=0;j<n;j++) {
				jpm_(i,j) = geometry_(i,0,j,0,0);
				jzz_(i,j) = geometry_(i,0,j,0,1);
			}
		}
	}

	~Heisenberg()
	{
		BaseType::deleteGarbage(garbage_);
	}

	SizeType size() const { return basis_.size(); }

	SizeType orbitals(SizeType) const
	{
		return 1;
	}

	void setupHamiltonian(SparseMatrixType& matrix) const
	{
		setupHamiltonian(matrix,basis_);
	}

	//! Gf. related functions below:
	void setupHamiltonian(SparseMatrixType& matrix,
	                      const BasisBaseType& basis) const
	{
		SizeType hilbert=basis.size();
		typename PsimagLite::Vector<RealType>::Type diag(hilbert,0.0);
		calcDiagonalElements(diag,basis);

		SizeType nsite = geometry_.numberOfSites();

		matrix.resize(hilbert,hilbert);
		// Calculate off-diagonal elements AND store matrix
		SizeType nCounter=0;
		for (SizeType ispace=0;ispace<hilbert;ispace++) {
			SparseRowType sparseRow;
			matrix.setRow(ispace,nCounter);
			WordType ket1 = basis(ispace,SPIN_UP);
			WordType ket2 = basis(ispace,SPIN_DOWN);
			//				std::cout<<"ket1="<<ket1<<" ket2="<<ket2<<"\n";
			// Save diagonal
			sparseRow.add(ispace,diag[ispace]);
			for (SizeType i=0;i<nsite;i++) {
				setSplusSminus(sparseRow,ket1,ket2,i,basis);
			}

			nCounter += sparseRow.finalize(matrix);
		}
		matrix.setRow(hilbert,nCounter);
		matrix.checkValidity();
		assert(isHermitian(matrix));
	}

	bool hasNewParts(std::pair<SizeType,SizeType>& newParts,
	                 SizeType what,
	                 SizeType spin,
	                 const PairType& orbs) const
	{
		if (what==ProgramGlobals::OPERATOR_SZ)
			return false;

		if (what == ProgramGlobals::OPERATOR_SPLUS ||
		    what == ProgramGlobals::OPERATOR_SMINUS)
			return hasNewPartsSplusOrMinus(newParts,what,spin,orbs);

		PsimagLite::String str(__FILE__);
		str += " " + ttos(__LINE__) +  "\n";
		str += PsimagLite::String("hasNewParts: unsupported operator ");
		str += ProgramGlobals::id2Operator(what) + "\n";
		throw std::runtime_error(str.c_str());
	}

	const GeometryType& geometry() const { return geometry_; }

	const BasisType& basis() const { return basis_; }

	void printBasis(std::ostream& os) const
	{
		os<<basis_;
	}

	PsimagLite::String name() const { return __FILE__; }

	BasisType* createBasis(SizeType nup, SizeType ndown) const
	{
		BasisType* ptr = new BasisType(geometry_,nup,ndown);
		garbage_.push_back(ptr);
		return ptr;
	}

	void print(std::ostream& os) const { os<<mp_; }

private:

	bool hasNewPartsSplusOrMinus(std::pair<SizeType,SizeType>& newParts,
	                             SizeType what,
	                             SizeType spin,
	                             const PairType&) const
	{
		throw PsimagLite::RuntimeError("BasisHeisenberg::hasNewPartsSplusOrMinus\n");
	}

	void calcDiagonalElements(typename PsimagLite::Vector<RealType>::Type& diag,
	                          const BasisBaseType& basis) const
	{
		SizeType hilbert=basis.size();
		SizeType nsite = geometry_.numberOfSites();
		SizeType orb = 0;

		// Calculate diagonal elements
		for (SizeType ispace=0;ispace<hilbert;ispace++) {
			WordType ket1 = basis(ispace,SPIN_UP);
			WordType ket2 = basis(ispace,SPIN_DOWN);
			ComplexOrRealType s=0;
			for (SizeType i=0;i<nsite;i++) {

				int niup = basis.isThereAnElectronAt(ket1,ket2,i,SPIN_UP,orb);

				int nidown = basis.isThereAnElectronAt(ket1,ket2,i,SPIN_DOWN,orb);

				if (i < mp_.magneticField.size())
					s += mp_.magneticField[i]*(niup-nidown);

				for (SizeType j=i+1;j<nsite;j++) {

					int njup = basis.isThereAnElectronAt(ket1,ket2,j,SPIN_UP,orb);
					int njdown = basis.isThereAnElectronAt(ket1,ket2,j,SPIN_DOWN,orb);

					// Sz Sz term:
					s += (niup-nidown) * (njup - njdown)  * jzz_(i,j)*0.25;
				}
			}

			assert(fabs(std::imag(s))<1e-12);
			diag[ispace] = std::real(s);
		}
	}

	void setSplusSminus(SparseRowType &sparseRow,
	                    const WordType& ket1,
	                    const WordType& ket2,
	                    SizeType i,
	                    const BasisBaseType &basis) const
	{
		WordType s1i=(ket1 & BasisType::bitmask(i));
		if (s1i>0) s1i=1;
		WordType s2i=(ket2 & BasisType::bitmask(i));
		if (s2i>0) s2i=1;

		SizeType nsite = geometry_.numberOfSites();

		// Hopping term
		for (SizeType j=0;j<nsite;j++) {
			if (j<i) continue;
			ComplexOrRealType h = jpm_(i,j)*0.5;
			if (std::real(h) == 0 && std::imag(h) == 0) continue;
			WordType s1j= (ket1 & BasisType::bitmask(j));
			if (s1j>0) s1j=1;
			WordType s2j= (ket2 & BasisType::bitmask(j));
			if (s2j>0) s2j=1;

			if (s1i==1 && s1j==0 && s2i==0 && s2j==1) {
				WordType bra1= ket1 ^ BasisType::bitmask(i);
				bra1 |= BasisType::bitmask(j);
				WordType bra2= ket2 | BasisType::bitmask(i);
				bra2 ^= BasisType::bitmask(j);
				SizeType temp = basis.perfectIndex(bra1,bra2);
				sparseRow.add(temp,h*signSplusSminus(i,j,bra1,bra2));
			}

			if (s1i==0 && s1j==1 && s2i==1 && s2j==0) {
				WordType bra1= ket1 | BasisType::bitmask(i);
				bra1 ^= BasisType::bitmask(j);
				WordType bra2= ket2 ^ BasisType::bitmask(i);
				bra2 |= BasisType::bitmask(j);
				SizeType temp = basis.perfectIndex(bra1,bra2);
				sparseRow.add(temp,h*signSplusSminus(i,j,bra1,bra2));
			}
		}
	}

	RealType signSplusSminus(SizeType i,
	                         SizeType j,
	                         const WordType& bra1,
	                         const WordType& bra2) const
	{
		SizeType n = geometry_.numberOfSites();
		int s = 1;
		if (j>0) s *= parityFrom(0,j-1,bra2);
		if (i>0) s *= parityFrom(0,i-1,bra2);
		if (i<n-1) s *= parityFrom(i+1,n-1,bra1);
		if (j<n-1) s *= parityFrom(j+1,n-1,bra1);
		return s;
	}

	// from i to j including i and j
	// assumes i<=j
	int parityFrom(SizeType i,SizeType j,const WordType& ket) const
	{
		if (i==j) return (BasisType::bitmask(j) & ket) ? -1 : 1;
		assert(i<j);
		//j>i>=0 now
		WordType mask = ket;
		mask &= ((1 << (i+1)) - 1) ^ ((1 << j) - 1);
		int s=(PsimagLite::BitManip::count(mask) & 1) ? -1 : 1;
		// Is there something of this species at i?
		if (BasisType::bitmask(i) & ket) s = -s;
		// Is there something of this species at j?
		if (BasisType::bitmask(j) & ket) s = -s;
		return s;
	}

	const ParametersModelType mp_;
	const GeometryType& geometry_;
	BasisType basis_;
	PsimagLite::Matrix<ComplexOrRealType> jpm_;
	PsimagLite::Matrix<ComplexOrRealType> jzz_;
	mutable typename PsimagLite::Vector<BasisType*>::Type garbage_;
}; // class Heisenberg
} // namespace LanczosPlusPlus
#endif
