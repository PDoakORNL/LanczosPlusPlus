
/*
Copyright (c) 2009, UT-Battelle, LLC
All rights reserved

[DMRG++, Version 2.0.0]
[by G.A., Oak Ridge National Laboratory]

UT Battelle Open Source Software License 11242008

OPEN SOURCE LICENSE

Subject to the conditions of this License, each
contributor to this software hereby grants, free of
charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), a
perpetual, worldwide, non-exclusive, no-charge,
royalty-free, irrevocable copyright license to use, copy,
modify, merge, publish, distribute, and/or sublicense
copies of the Software.

1. Redistributions of Software must retain the above
copyright and license notices, this list of conditions,
and the following disclaimer.  Changes or modifications
to, or derivative works of, the Software should be noted
with comments and the contributor and organization's
name.

2. Neither the names of UT-Battelle, LLC or the
Department of Energy nor the names of the Software
contributors may be used to endorse or promote products
derived from this software without specific prior written
permission of UT-Battelle.

3. The software and the end-user documentation included
with the redistribution, with or without modification,
must include the following acknowledgment:

"This product includes software produced by UT-Battelle,
LLC under Contract No. DE-AC05-00OR22725  with the
Department of Energy."
 
*********************************************************
DISCLAIMER

THE SOFTWARE IS SUPPLIED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER, CONTRIBUTORS, UNITED STATES GOVERNMENT,
OR THE UNITED STATES DEPARTMENT OF ENERGY BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED
STATES DEPARTMENT OF ENERGY, NOR THE COPYRIGHT OWNER, NOR
ANY OF THEIR EMPLOYEES, REPRESENTS THAT THE USE OF ANY
INFORMATION, DATA, APPARATUS, PRODUCT, OR PROCESS
DISCLOSED WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS.

*********************************************************
*/

#ifndef HUBBARDLANCZOS_H
#define HUBBARDLANCZOS_H

#include "CrsMatrix.h"
#include "BasisHubbardLanczos.h"

namespace Dmrg {
	
	template<typename RealType_,typename ParametersType,
		typename GeometryType>
	class HubbardLanczos {
		
		typedef psimag::Matrix<RealType_> MatrixType; 
	public:
		
		typedef BasisHubbardLanczos BasisType;
		typedef typename BasisType::WordType WordType;
		typedef RealType_ RealType;
		typedef CrsMatrix<RealType> SparseMatrixType;
		typedef std::vector<RealType> VectorType;
		enum {SPIN_UP,SPIN_DOWN};
		enum {DESTRUCTOR,CONSTRUCTOR};
		
		
		HubbardLanczos(size_t nup,size_t ndown,const ParametersType& mp,GeometryType& geometry)
			: mp_(mp),geometry_(geometry),
			  basis1_(geometry.numberOfSites(),nup),basis2_(geometry.numberOfSites(),ndown)
		{
		}
		
		

			size_t size() const { return basis1_.size()*basis2_.size(); }
		

		void setupHamiltonian(SparseMatrixType &matrix) const
		{
			setupHamiltonian(matrix,basis1_,basis2_);
		}
		

		void getOperator(SparseMatrixType& matrix,size_t what,size_t i,size_t sector) const
		{
			size_t hilbert1 = basis1_.size();
			size_t hilbert2 = basis2_.size();

			matrix.resize(hilbert1*hilbert2);

			size_t nCounter = 0;
			for (size_t ispace1=0;ispace1<hilbert1;ispace1++) {
				WordType ket1 = basis1_[ispace1];
				for (size_t ispace2=0;ispace2<hilbert2;ispace2++) {
					matrix.setRow(ispace2+hilbert2*ispace1,nCounter);

					WordType ket2 = basis2_[ispace2];
					WordType bra1 = ket1;
					WordType bra2 = ket2;
					if (sector==SPIN_DOWN) {
						// modify bra1
						if (!getBra(bra1,ket1,what,i)) continue;
					} else {
						//modify bra2:
						if (!getBra(bra2,ket2,what,i)) continue;
					}
					size_t temp = perfectIndex(basis1_,basis2_,bra1,bra2);
					matrix.pushCol(temp);
					RealType cTemp=doSign(ket1,ket2,i,sector); // check SIGN FIXME

					matrix.pushValue(cTemp);
					nCounter++;
				}
			}
			matrix.setRow(hilbert1*hilbert2,nCounter);
		}
		
		
	private:
		

		bool getBra(WordType& bra, const WordType& ket,size_t what,size_t i) const
		{
			WordType s1i=(ket & BasisType::bitmask(i));
			if (what==DESTRUCTOR) {
				if (s1i>0) {
					bra = (ket ^ BasisType::bitmask(i));
				} else {
					return false; // cannot destroy, there's nothing
				}
			} else {
				if (s1i==0) {
					bra = (ket ^ BasisType::bitmask(i));
				} else {
					return false; // cannot contruct, there's already one
				}
			}
			return true;
		}
		

		RealType hoppings(size_t i,size_t j) const
		{
			return geometry_(i,0,j,0,0);
		}
		

		void setupHamiltonian(SparseMatrixType &matrix,const BasisType &basis1,const BasisType& basis2) const
		{
			// Calculate diagonal elements AND count non-zero matrix elements
			size_t hilbert1=basis1.size();
			size_t hilbert2=basis2.size();
			MatrixType diag(hilbert2,hilbert1);
			size_t nzero = countNonZero(diag,basis1,basis2);
			
			size_t nsite = geometry_.numberOfSites();
			
			// Setup CRS matrix
			matrix.resize(hilbert1*hilbert2,nzero);
			
			// Calculate off-diagonal elements AND store matrix
			size_t nCounter=0;
			matrix.setRow(0,0);
			for (size_t ispace1=0;ispace1<hilbert1;ispace1++) {
				WordType ket1 = basis1[ispace1];
				for (size_t ispace2=0;ispace2<hilbert2;ispace2++) {
					WordType ket2 = basis2[ispace2];
					// Save diagonal
					matrix.setCol(nCounter,ispace2+ispace1*hilbert2);
					RealType cTemp=diag(ispace2,ispace1);
					matrix.setValues(nCounter,cTemp);
					nCounter++;
					for (size_t i=0;i<nsite;i++) {
						WordType s1i=(ket1 & BasisType::bitmask(i));
						WordType s2i=(ket2 & BasisType::bitmask(i));
						if (s1i>0) s1i=1;
						if (s2i>0) s2i=1;
						
						// Hopping term 
						for (size_t j=0;j<nsite;j++) {
							if (j<i) continue;
							RealType tmp = hoppings(i,j);
							if (tmp==0) continue;
							WordType s1j= (ket1 & BasisType::bitmask(j));
							WordType s2j= (ket2 & BasisType::bitmask(j));
							if (s1j>0) s1j=1;
							if (s2j>0) s2j=1;
							if (s1i+s1j==1) {
								WordType bra1= ket1 ^(BasisType::bitmask(i)|BasisType::bitmask(j));
								size_t temp = perfectIndex(basis1,basis2,bra1,ket2);
								matrix.setCol(nCounter,temp);
								cTemp=hoppings(i,j)*doSign(ket1,ket2,i,j,SPIN_DOWN); // check SIGN FIXME
								if (cTemp==0.0) {
									std::cerr<<"ctemp=0 and hopping="<<hoppings(i,j)<<" and i="<<i<<" and j="<<j<<"\n";
								}
								matrix.setValues(nCounter,cTemp);
								nCounter++;
							}
							if (s2i+s2j==1) {
								WordType bra2= ket2 ^(BasisType::bitmask(i)|BasisType::bitmask(j));
								size_t temp = perfectIndex(basis1,basis2,ket1,bra2);
								matrix.setCol(nCounter,temp);
								cTemp=hoppings(i,j)*doSign(ket1,ket2,i,j,SPIN_UP); // Check SIGN FIXME
								matrix.setValues(nCounter,cTemp);
								nCounter++;					
							}
						}
					}
					matrix.setRow(ispace2+hilbert2*ispace1+1,nCounter);
				}
			}
			matrix.setRow(hilbert1*hilbert2,nCounter);
		}
		

		size_t countNonZero(MatrixType& diag,const BasisType &basis1,const BasisType& basis2) const
		{
			size_t hilbert1=basis1.size();
			size_t hilbert2=basis2.size();
			size_t nsite = geometry_.numberOfSites();

			// Calculate diagonal elements AND count non-zero matrix elements
			size_t nzero = 0;
			for (size_t ispace1=0;ispace1<hilbert1;ispace1++) {
				WordType ket1 = basis1[ispace1];
				for (size_t ispace2=0;ispace2<hilbert2;ispace2++) {
					WordType ket2 = basis2[ispace2];
					RealType s=0;
					for (size_t i=0;i<nsite;i++) {
						WordType s1i=(ket1 & BasisType::bitmask(i));
						WordType s2i=(ket2 & BasisType::bitmask(i));
						if (s1i>0) s1i=1;
						if (s2i>0) s2i=1;
						
						// Hubbard term
						if (s1i>0 && s2i>0 ) s += mp_.hubbardU[i];
								
						// Potential term
						if (s1i>0) s += mp_.potentialV[i];
						if (s2i>0) s += mp_.potentialV[i];
						
						// Hopping term (only count how many non-zero)
						for (size_t j=0;j<nsite;j++) {
							if (j<i) continue;
							RealType tmp = hoppings(i,j);
							if (tmp==0) continue;
							
							WordType s1j= (ket1 & BasisType::bitmask(j));
							WordType s2j= (ket2 & BasisType::bitmask(j));
							if (s1j>0) s1j=1;
							if (s2j>0) s2j=1;
							if (s1i+s1j==1) nzero++;
							if (s2i+s2j==1) nzero++;
						}
					}
					// cout<<"diag of ("<<ispace1<<","<<ispace2<<"): "<<s<<endl;
					diag(ispace2,ispace1)=s;
					nzero++;
				}
			}

			nzero++;
			return nzero;
		}
		

		size_t perfectIndex(const BasisType& basis1,const BasisType& basis2,WordType ket1,WordType ket2) const
		{
			size_t hilbert2=basis2.size();
			size_t n1 = basis1.perfectIndex(ket1);
			size_t n2 = basis2.perfectIndex(ket2);

			return n2 + n1*hilbert2;
		}
		

		int doSign(WordType a, WordType b,size_t i,size_t j,size_t sector) const
		{
			if (i > j) {
				std::cerr<<"FATAL: At doSign\n";
				std::cerr<<"INFO: i="<<i<<" j="<<j<<std::endl;
				std::cerr<<"AT: "<<__FILE__<<" : "<<__LINE__<<std::endl;
				throw std::runtime_error("HubbardLanczos::doSign(...)\n");
			}

			WordType mask = a ^  b;
			mask &= ((1 << (i+1)) - 1) ^ ((1 << j) - 1);
			int s=(BasisType::bitcnt (mask) & 1) ? -1 : 1; // Parity of single occupied between i and j

			if (sector==SPIN_DOWN) { // Is there a down at j?
				if (BasisType::bitmask(j) & b) s = -s;
			}
			if (sector==SPIN_UP) { // Is there an up at i?
				if (BasisType::bitmask(i) & a) s = -s;
			}

			return s;
		}
		

		int doSign(WordType a, WordType b,size_t i,size_t sector) const
		{
			size_t nsite = geometry_.numberOfSites();
			if (i<nsite-1) {
				WordType mask = a ^ b;
				mask &= ((1 << (i+1)) - 1) ^ ((1 << nsite) - 1);
				int s=(BasisType::bitcnt (mask) & 1) ? -1 : 1; // Parity of single occupied between i and nsite-1
				//cout<<"sign1: "<<s<<"a="<<a<<" b="<<b<<"sector="<<sector<<endl;
				if (sector==SPIN_UP) { // Is there an up at i?
					if (BasisType::bitmask(i) & a) s = -s;
					//cout<<"sign2: "<<s<<endl;
				}
				return s;
			}

			int s=1;
			if (sector==SPIN_UP) { // Is there an up at i?
				if (BasisType::bitmask(i) & a) s = -s;
			}

			return s;
		}
		
		
		
		const ParametersType& mp_;
		const GeometryType& geometry_;
		BasisType basis1_;
		BasisType basis2_;
		
	}; // class HubbardLanczos 
} // namespace
#endif

