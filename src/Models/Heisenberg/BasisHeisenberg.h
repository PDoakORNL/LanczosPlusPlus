/*
*/

#ifndef LANCZOS_BASIS_HEISENBERG_H
#define LANCZOS_BASIS_HEISENBERG_H

#include "BitManip.h"
#include "ProgramGlobals.h"
#include "../../Engine/BasisBase.h"

namespace LanczosPlusPlus {

template<typename GeometryType>
class BasisHeisenberg : public BasisBase<GeometryType> {

	typedef ProgramGlobals::PairIntType PairIntType;

public:

	typedef BasisBase<GeometryType> BaseType;
	typedef typename BaseType::WordType WordType;
	typedef typename BaseType::VectorWordType VectorWordType;
	static VectorWordType bitmask_;

	enum {OPERATOR_NIL=ProgramGlobals::OPERATOR_NIL,
	      OPERATOR_SZ=ProgramGlobals::OPERATOR_SZ};

	BasisHeisenberg(const GeometryType& geometry,
	                SizeType twiceS,
	                SizeType szPlusConst)
	    : geometry_(geometry), twiceS_(twiceS), szPlusConst_(szPlusConst)
	{
		assert(bitmask_.size()==0 || bitmask_.size()==geometry_.numberOfSites());
		if (bitmask_.size()==0) doBitmask();
		throw PsimagLite::RuntimeError("BasisHeisenberg::ctor: setup data here\n");
	}

	static const WordType& bitmask(SizeType i)
	{
		return bitmask_[i];
	}

	SizeType size() const { return data_.size(); }

	//! Spins
	SizeType dofs() const { return twiceS_ + 1; }

	SizeType perfectIndex(const VectorWordType& kets) const
	{
		throw PsimagLite::RuntimeError("BasisHeisenberg::perfectIndex kets\n");
	}

	SizeType perfectIndex(WordType ket1,WordType ket2) const
	{
		throw PsimagLite::RuntimeError("BasisHeisenberg::perfectIndex ket1 ket2\n");
	}

	WordType operator()(SizeType i,SizeType spin) const
	{
		throw PsimagLite::RuntimeError("BasisHeisenberg::operator()\n");
	}

	SizeType isThereAnElectronAt(WordType ket1,
	                             WordType ket2,
	                             SizeType site,
	                             SizeType spin,
	                             SizeType) const
	{
		throw PsimagLite::RuntimeError("BasisHeisenberg::isThereAnElectronAt\n");
	}

	SizeType getN(WordType ket1,
	              WordType ket2,
	              SizeType site,
	              SizeType spin,
	              SizeType) const
	{
		throw PsimagLite::RuntimeError("BasisHeisenberg::getN");
	}

	int doSignGf(WordType a, WordType b,SizeType ind,SizeType sector,SizeType) const
	{
		throw PsimagLite::RuntimeError("BasisHeisenberg::doSignGf\n");
	}

	int doSign(WordType ket1,
	           WordType ket2,
	           SizeType i,
	           SizeType,
	           SizeType j,
	           SizeType,
	           SizeType spin) const
	{
		throw PsimagLite::RuntimeError("BasisHeisenberg::doSign\n");
	}

	PairIntType getBraIndex(WordType ket1,
	                        WordType ket2,
	                        SizeType operatorLabel,
	                        SizeType site,
	                        SizeType spin,
	                        SizeType orb) const
	{
		if (operatorLabel == ProgramGlobals::OPERATOR_SPLUS) {
			throw PsimagLite::RuntimeError("BasisHeisenberg::getBraIndex SPLUS\n");

		} else if (operatorLabel == ProgramGlobals::OPERATOR_SMINUS) {
			throw PsimagLite::RuntimeError("BasisHeisenberg::getBraIndex SMINUS\n");
		}

		return getBraIndex_(ket1,ket2,operatorLabel,site,spin,orb);
	}

	SizeType orbsPerSite(SizeType) const { return 1; }

	SizeType orbs() const { return 1; }

	SizeType perfectIndex(WordType,SizeType,SizeType) const
	{
		throw PsimagLite::RuntimeError("BasisHeisenberg::perfectIndex\n");
	}

	void print(std::ostream& os) const
	{
		SizeType hilbert = 1;
		hilbert <<= geometry_.numberOfSites();
		ProgramGlobals::printBasisVector(os,hilbert,data_);
	}

	template<typename GeometryType2>
	friend std::ostream& operator<<(std::ostream& os,
	                                const BasisHeisenberg<GeometryType2>& basis);

private:

	PairIntType getBraIndex_(const WordType& ket1,
	                         const WordType& ket2,
	                         SizeType operatorLabel,
	                         SizeType site,
	                         SizeType spin,
	                         SizeType) const
	{
		throw PsimagLite::RuntimeError("BasisHeisenberg::getBraIndex_ \n");
	}

	int getBra(WordType& bra,
	           SizeType operatorLabel,
	           const WordType& ket1,
	           const WordType& ket2,
	           //		            SizeType what,
	           SizeType site,
	           SizeType spin) const
	{
		switch(operatorLabel) {
		case ProgramGlobals::OPERATOR_SZ:
			return getBraSzOrN(bra,ket1,ket2,operatorLabel,site,spin);
		}

		assert(false);
		return 0;
	}

	void doBitmask()
	{
		SizeType n = geometry_.numberOfSites();
		bitmask_.resize(n);
		bitmask_[0]=1ul;
		for (SizeType i=1;i<n;i++)
			bitmask_[i] = bitmask_[i-1]<<1;
	}

	int doSign(WordType ket,SizeType i,SizeType j) const
	{
		throw PsimagLite::RuntimeError("BasisHeisenberg::doSign \n");
	}

	int getBraSzOrN(WordType& bra,
	                const WordType& ket1,
	                const WordType& ket2,
	                SizeType,
	                SizeType site,
	                SizeType spin) const
	{
		throw PsimagLite::RuntimeError("BasisHeisenberg::doSign \n");
	}

	const GeometryType& geometry_;
	SizeType twiceS_;
	SizeType szPlusConst_;
	VectorWordType data_;
}; // class BasisHeisenberg

template<typename GeometryType>
std::ostream& operator<<(std::ostream& os,const BasisHeisenberg<GeometryType>& basis)
{
	for (SizeType i=0;i<basis.data_.size();i++)
		os<<i<<" "<<basis.data_[i]<<"\n";
	return os;
}

template<typename GeometryType>
typename BasisHeisenberg<GeometryType>::VectorWordType
BasisHeisenberg<GeometryType>::bitmask_;

} // namespace LanczosPlusPlus
#endif
