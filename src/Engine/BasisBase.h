
/*
// BEGIN LICENSE BLOCK
Copyright (c) 2014 , UT-Battelle, LLC
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

#ifndef LANCZOS_BASIS_BASE_H
#define LANCZOS_BASIS_BASE_H
#include "Vector.h"

namespace LanczosPlusPlus {

template<typename GeometryType>
class BasisBase {

public:

	typedef ProgramGlobals::PairIntType PairIntType;
	typedef ProgramGlobals::WordType WordType;
	typedef typename PsimagLite::Vector<WordType>::Type VectorWordType;

	virtual ~BasisBase() {}

	virtual SizeType dofs() const = 0;

	virtual SizeType size() const = 0;

	virtual WordType operator()(SizeType i,SizeType spin) const = 0;

	virtual SizeType perfectIndex(const PsimagLite::Vector<WordType>::Type& kets) const = 0;

	virtual SizeType perfectIndex(WordType ket1,WordType ket2) const = 0;

	virtual SizeType perfectIndex(WordType newKet,
	                              SizeType ispace,
	                              SizeType spinOfNew) const = 0;

	virtual PairIntType getBraIndex(WordType ket1,
	                        WordType ket2,
	                        SizeType what,
	                        SizeType site,
	                        SizeType spin,
	                        SizeType orb) const = 0;

	virtual int doSign(WordType ket1,
	           WordType ket2,
	           SizeType i,
	           SizeType orb1,
	           SizeType j,
	           SizeType orb2,
	           SizeType spin) const = 0;

	virtual int doSignGf(WordType a,
	                     WordType b,
	                     SizeType ind,
	                     SizeType spin,
	                     SizeType orb) const = 0;

	virtual SizeType isThereAnElectronAt(WordType ket1,
	                                     WordType ket2,
	                                     SizeType site,
	                                     SizeType spin,
	                                     SizeType orb) const = 0;

	virtual SizeType orbsPerSite(SizeType i) const = 0;

	virtual SizeType orbs() const = 0;

	virtual SizeType getN(WordType ket1,
	                      WordType ket2,
	                      SizeType site,
	                      SizeType spin,
	                      SizeType orb) const = 0;

	virtual WordType getBra(WordType,
	                        SizeType,
	                        SizeType,
	                        SizeType,
	                        SizeType) const
	{
		throw PsimagLite::RuntimeError("getBra unimplemented in base class\n");
	}

	virtual void print(std::ostream&) const = 0;
}; // class BasisBase

} // namespace LanczosPlusPlus
#endif

