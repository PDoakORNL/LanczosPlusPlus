
/*
// BEGIN LICENSE BLOCK
Copyright (c) 2009 , UT-Battelle, LLC
All rights reserved

[Lanczos++, Version 1.0.0]

*********************************************************
THE SOFTWARE IS SUPPLIED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. 

Please see full open source license included in file LICENSE.
*********************************************************

*/

#ifndef BASIS_FEASBASED_SC_H
#define BASIS_FEASBASED_SC_H
#include "BasisOneSpinFeAs.h"

namespace LanczosPlusPlus {

	template<typename GeometryType>
	class BasisFeAsBasedSc {

		typedef ProgramGlobals::PairIntType PairIntType;

		static size_t orbitals_;

	public:
		
		typedef BasisOneSpinFeAs BasisType;
		typedef BasisType::WordType WordType;
		static int const FERMION_SIGN = BasisType::FERMION_SIGN;	
		
		BasisFeAsBasedSc(const GeometryType& geometry, size_t nup,size_t ndown,size_t orbitals)
		: basis1_(geometry.numberOfSites(),nup,orbitals),
		  basis2_(geometry.numberOfSites(),ndown,orbitals)
		{
			orbitals_ = orbitals;
//			std::cout<<"Basis1\n";
//			std::cout<<basis1_;
//			std::cout<<"Basis2\n";
//			std::cout<<basis2_;
		}
		
		BasisFeAsBasedSc(const GeometryType& geometry, size_t nup,size_t ndown)
		: basis1_(geometry.numberOfSites(),nup,orbitals_),
		  basis2_(geometry.numberOfSites(),ndown,orbitals_)
		{}
		

		static const WordType& bitmask(size_t i)
		{
			return BasisType::bitmask(i);
		}
		
		size_t dofs() const { return 2*orbitals_; }

		size_t size() const { return basis1_.size()*basis2_.size(); }
		
		const WordType& operator()(size_t i,size_t spin) const
		{
			size_t y = i/basis1_.size();
			size_t x = i%basis1_.size();
			return (spin==ProgramGlobals::SPIN_UP) ? basis1_[x] : basis2_[y];
		}

		size_t perfectIndex(const PsimagLite::Vector<WordType>::Type& kets) const
		{
			assert(kets.size()==2);
			return  perfectIndex(kets[0],kets[1]);
		}

		size_t perfectIndex(WordType ket1,WordType ket2) const
		{
			return basis1_.perfectIndex(ket1) + basis2_.perfectIndex(ket2)*basis1_.size();
		}
		

		size_t getN(size_t i,size_t spin,size_t orb) const
		{
			size_t y = i/basis1_.size();
			size_t x = i%basis1_.size();
			return (spin==ProgramGlobals::SPIN_UP) ? basis1_.getN(x,orb) : basis2_.getN(y,orb);
		}

		size_t getN(WordType ket,size_t site,size_t spin,size_t orb) const
		{
			return (spin==ProgramGlobals::SPIN_UP) ? basis1_.getN(ket,site,orb) : basis2_.getN(ket,site,orb);
		}

		PairIntType getBraIndex(WordType ket1, WordType ket2,size_t what,size_t site,size_t spin,size_t orb) const
		{
			if (what==ProgramGlobals::OPERATOR_C ||
			    what==ProgramGlobals::OPERATOR_CDAGGER ||
			    what==ProgramGlobals::OPERATOR_N)
				return PairIntType(getBraIndexCorCdaggerOrN(ket1,ket2,what,site,spin,orb),1);
			if (what==ProgramGlobals::OPERATOR_SPLUS || what==ProgramGlobals::OPERATOR_SMINUS)
				return PairIntType(getBraIndexSplusOrSminus(ket1,ket2,what,site,orb),1);
			PsimagLite::String str(__FILE__);
			str += " " + ttos(__LINE__) +  "\n";
			str += PsimagLite::String("getBraIndex: unsupported operator ");
			str += ProgramGlobals::id2Operator(what) + "\n";
			throw std::runtime_error(str.c_str());
		}

		int doSign(WordType ket1,
			   WordType ket2,
			   size_t i,
			   size_t orb1,
			   size_t j,
			   size_t orb2,
			   size_t spin) const
		{
			if (i > j) {
				std::cerr<<"FATAL: At doSign\n";
				std::cerr<<"INFO: i="<<i<<" j="<<j<<std::endl;
				std::cerr<<"AT: "<<__FILE__<<" : "<<__LINE__<<std::endl;
				throw std::runtime_error("FeBasedSc::doSign(...)\n");
			}
			if (spin==ProgramGlobals::SPIN_UP) {
				return basis1_.doSign(ket1,i,orb1,j,orb2);
			}
			return basis2_.doSign(ket2,i,orb1,j,orb2);
		}

		int doSignGf(WordType a, WordType b,size_t ind,size_t spin,size_t orb) const
		{
			if (spin==ProgramGlobals::SPIN_UP) return basis1_.doSignGf(a,ind,orb);

			int s=(PsimagLite::BitManip::count(a) & 1) ? -1 : 1; // Parity of up
			int s2 = basis2_.doSignGf(b,ind,orb);

			return s*s2;
		}

		size_t isThereAnElectronAt(
				size_t ket1,
				size_t ket2,
				size_t site,
				size_t spin,
				size_t orb) const
		{
			if (spin==ProgramGlobals::SPIN_UP)
				return basis1_.isThereAnElectronAt(ket1,site,orb);
			return basis2_.isThereAnElectronAt(ket2,site,orb);
		}

		bool hasNewParts(std::pair<size_t,size_t>& newParts,
		                 size_t what,
		                 size_t spin,
		                 const std::pair<size_t,size_t>& orbs) const
		{
			if (what==ProgramGlobals::OPERATOR_C || what==ProgramGlobals::OPERATOR_CDAGGER)
				return hasNewPartsCorCdagger(newParts,what,spin,orbs);
			if (what==ProgramGlobals::OPERATOR_SPLUS || what==ProgramGlobals::OPERATOR_SMINUS)
				return hasNewPartsSplusOrSminus(newParts,what,orbs);
			PsimagLite::String str(__FILE__);
			str += " " + ttos(__LINE__) +  "\n";
			str += PsimagLite::String("hasNewParts: unsupported operator ");
			str += ProgramGlobals::id2Operator(what) + "\n";
			throw std::runtime_error(str.c_str());
		}

	private:

		int getBraIndexCorCdaggerOrN(WordType ket1, WordType ket2,size_t what,size_t site,size_t spin,size_t orb) const
		{

			WordType bra  =0;
			bool b = getBraCorCdaggerOrN(bra,ket1,ket2,what,site,spin,orb);
			if (!b) return -1;
			return (spin==ProgramGlobals::SPIN_UP) ? perfectIndex(bra,ket2) :
			                         perfectIndex(ket1,bra);
		}

		int getBraIndexSplusOrSminus(WordType ket1, WordType ket2,size_t what,size_t site,size_t orb) const
		{

			WordType bra1  =0;
			WordType bra2  =0;
			bool b = getBraSplusOrSminus(bra1,bra2,ket1,ket2,what,site,orb);
			if (!b) return -1;
			return perfectIndex(bra1,bra2);
		}

		bool hasNewPartsCorCdagger(std::pair<size_t,size_t>& newParts,
		                           size_t what,
		                           size_t spin,
		                           const std::pair<size_t,size_t>& orbs) const
		{
			int newPart1=basis1_.electrons();
			int newPart2=basis2_.electrons();

			if (spin==ProgramGlobals::SPIN_UP) newPart1 = basis1_.newPartCorCdagger(what,orbs.first);
			else newPart2 = basis2_.newPartCorCdagger(what,orbs.second);

			if (newPart1<0 || newPart2<0) return false;

			if (newPart1==0 && newPart2==0) return false;
			newParts.first = size_t(newPart1);
			newParts.second = size_t(newPart2);
			return true;
		}

		bool hasNewPartsSplusOrSminus(std::pair<size_t,size_t>& newParts,
		                              size_t what,
		                              const std::pair<size_t,size_t>& orbs) const
		{
			int c1 = (what==ProgramGlobals::OPERATOR_SPLUS) ? 1 : -1;
			int c2 = (what==ProgramGlobals::OPERATOR_SPLUS) ? -1 : 1;

			int newPart1 = basis1_.hasNewPartsSplusOrSminus(c1,orbs.first);
			int newPart2 = basis2_.hasNewPartsSplusOrSminus(c2,orbs.second);

			if (newPart1<0 || newPart2<0) return false;

			if (newPart1==0 && newPart2==0) return false;
			newParts.first = size_t(newPart1);
			newParts.second = size_t(newPart2);
			return true;
		}
		
		bool getBraCorCdaggerOrN(WordType& bra,
		                         const WordType& ket1,
		                         const WordType& ket2,
		                         size_t what,
		                         size_t site,
		                         size_t spin,
		                         size_t orb) const
		{
			return (spin==ProgramGlobals::SPIN_UP) ? basis1_.getBra(bra,ket1,what,site,orb) :
			                         basis2_.getBra(bra,ket2,what,site,orb);
		}

		bool getBraSplusOrSminus(WordType& bra1,
		                        WordType& bra2,
		                        const WordType& ket1,
		                        const WordType& ket2,
		                        size_t what,
		                        size_t site,
		                        size_t orb) const
		{
			size_t what1 = (what==ProgramGlobals::OPERATOR_SPLUS) ? ProgramGlobals::OPERATOR_CDAGGER : ProgramGlobals::OPERATOR_C;
			size_t what2 = (what==ProgramGlobals::OPERATOR_SPLUS) ? ProgramGlobals::OPERATOR_C : ProgramGlobals::OPERATOR_CDAGGER;
			bool b1 = basis1_.getBra(bra1,ket1,what1,site,orb);
			bool b2 = basis2_.getBra(bra2,ket2,what2,site,orb);
			return (b1 & b2);
		}

		BasisType basis1_,basis2_;
	}; // class BasisFeAsBasedSc

	template<typename GeometryType>
	size_t BasisFeAsBasedSc<GeometryType>::orbitals_=2;

} // namespace LanczosPlusPlus
#endif

