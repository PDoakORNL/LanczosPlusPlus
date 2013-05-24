
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

#ifndef FEBASED_SC_H
#define FEBASED_SC_H

#include "CrsMatrix.h"
#include "BasisFeAsBasedSc.h"
#include "SparseRow.h"
#include "ParametersModelFeAs.h"

namespace LanczosPlusPlus {
	
	template<typename RealType_,typename GeometryType_>
	class FeBasedSc {
		
		typedef PsimagLite::Matrix<RealType_> MatrixType;

	public:

		typedef ParametersModelFeAs<RealType_> ParametersModelType;
		typedef GeometryType_ GeometryType;
		typedef BasisFeAsBasedSc<GeometryType> BasisType;
		typedef typename BasisType::WordType WordType;
		typedef RealType_ RealType;
		typedef PsimagLite::CrsMatrix<RealType> SparseMatrixType;
		typedef PsimagLite::SparseRow<SparseMatrixType> SparseRowType;
		typedef typename PsimagLite::Vector<RealType>::Type VectorType;
		enum {TERM_HOPPINGS=0,TERM_J=1};
		static int const FERMION_SIGN = BasisType::FERMION_SIGN;
		
		
		FeBasedSc(SizeType nup,SizeType ndown,const ParametersModelType& mp,const GeometryType& geometry)
		: mp_(mp),geometry_(geometry),basis_(geometry,nup,ndown,mp_.orbitals)
		{
		}
		
		SizeType size() const { return basis_.size(); }

		SizeType orbitals(SizeType site) const
		{
			return mp_.orbitals;
		}

		void setupHamiltonian(SparseMatrixType &matrix) const
		{
			setupHamiltonian(matrix,basis_);
		}
		
		bool hasNewParts(std::pair<SizeType,SizeType>& newParts,
		                 SizeType what,
		                 SizeType spin,
		                 const std::pair<SizeType,SizeType>& orbs) const
		{
			return basis_.hasNewParts(newParts,what,spin,orbs);
		}

		const GeometryType& geometry() const { return geometry_; }

		void setupHamiltonian(SparseMatrixType &matrix,
				      const BasisType &basis) const
		{
			// Calculate diagonal elements AND count non-zero matrix elements
			SizeType hilbert=basis.size();
			typename PsimagLite::Vector<RealType>::Type diag(hilbert);
			calcDiagonalElements(diag,basis);

			SizeType nsite = geometry_.numberOfSites();

			// Setup CRS matrix
			matrix.resize(hilbert,hilbert);

			// Calculate off-diagonal elements AND store matrix
			SizeType nCounter=0;
			for (SizeType ispace=0;ispace<hilbert;ispace++) {
				SparseRowType sparseRow;
				matrix.setRow(ispace,nCounter);
				WordType ket1 = basis(ispace,ProgramGlobals::SPIN_UP);
				WordType ket2 = basis(ispace,ProgramGlobals::SPIN_DOWN);
				// Save diagonal
				sparseRow.add(ispace,diag[ispace]);
				for (SizeType i=0;i<nsite;i++) {
					for (SizeType orb=0;orb<mp_.orbitals;orb++) {
						setHoppingTerm(sparseRow,ket1,ket2,
								i,orb,basis);

						setU2OffDiagonalTerm(sparseRow,ket1,ket2,
							i,orb,basis);
						for (SizeType orb2=0;orb2<mp_.orbitals;orb2++) {
							if (orb==orb2) continue;

							setU3Term(sparseRow,ket1,ket2,
									  i,orb,orb2,basis);
						}

						setJTermOffDiagonal(sparseRow,ket1,ket2,
								i,orb,basis);
					}
				}
				nCounter += sparseRow.finalize(matrix);
			}
			matrix.setRow(hilbert,nCounter);
		}

		void matrixVectorProduct(VectorType &x,const VectorType& y) const
		{
			matrixVectorProduct(x,y,&basis_);
		}

		void matrixVectorProduct(VectorType &x,
					  const VectorType& y,
					  const BasisType* basis) const
		{
			// Calculate diagonal elements AND count non-zero matrix elements
			SizeType hilbert=basis->size();
			calcDiagonalElements(x,y,basis);

			SizeType nsite = geometry_.numberOfSites();

			// Calculate off-diagonal elements AND store matrix

			for (SizeType ispace=0;ispace<hilbert;ispace++) {
				SparseRowType sparseRow;

				WordType ket1 = basis->operator ()(ispace,ProgramGlobals::SPIN_UP);
				WordType ket2 = basis->operator ()(ispace,ProgramGlobals::SPIN_DOWN);

				//x[ispace] += diag[ispace]*y[ispace];
				for (SizeType i=0;i<nsite;i++) {
					for (SizeType orb=0;orb<mp_.orbitals;orb++) {
						setHoppingTerm(sparseRow,ket1,ket2,
								i,orb,*basis);

						setU2OffDiagonalTerm(sparseRow,ket1,ket2,
								i,orb,*basis);

						for (SizeType orb2=orb+1;orb2<mp_.orbitals;orb2++) {
							setU3Term(sparseRow,ket1,ket2,
								i,orb,orb2,*basis);
						}
						setJTermOffDiagonal(sparseRow,ket1,ket2,
								i,orb,*basis);
					}
				}
				x[ispace] += sparseRow.finalize(y);
			}
		}

		const BasisType& basis() const { return basis_; }

		PsimagLite::String name() const { return __FILE__; }

	private:

		RealType hoppings(SizeType i,SizeType orb1,SizeType j,SizeType orb2) const
		{
			return -geometry_(i,orb1,j,orb2,TERM_HOPPINGS);
		}

		void setHoppingTerm(
				SparseRowType &sparseRow,
				const WordType& ket1,
				const WordType& ket2,
				SizeType i,
				SizeType orb,
				const BasisType &basis) const
		{
			SizeType ii = i*mp_.orbitals+orb;
			WordType s1i=(ket1 & BasisType::bitmask(ii));
			if (s1i>0) s1i=1;
			WordType s2i=(ket2 & BasisType::bitmask(ii));
			if (s2i>0) s2i=1;

			SizeType nsite = geometry_.numberOfSites();

			// Hopping term
			for (SizeType j=0;j<nsite;j++) {
				if (j<i) continue;
				for (SizeType orb2=0;orb2<mp_.orbitals;orb2++) {
					SizeType jj = j*mp_.orbitals+orb2;
					RealType h = hoppings(i,orb,j,orb2);
					if (h==0) continue;
					WordType s1j= (ket1 & BasisType::bitmask(jj));
					if (s1j>0) s1j=1;
					WordType s2j= (ket2 & BasisType::bitmask(jj));
					if (s2j>0) s2j=1;

					if (s1i+s1j==1) {
						WordType bra1= ket1 ^(BasisType::bitmask(ii)|BasisType::bitmask(jj));
						SizeType temp = basis.perfectIndex(bra1,ket2);
						int extraSign = (s1i==1) ? FERMION_SIGN : 1;
						RealType cTemp = h*extraSign*basis_.doSign(
							ket1,ket2,i,orb,j,orb2,ProgramGlobals::SPIN_UP);
						sparseRow.add(temp,cTemp);

					}
					if (s2i+s2j==1) {
						WordType bra2= ket2 ^(BasisType::bitmask(ii)|BasisType::bitmask(jj));
						SizeType temp = basis.perfectIndex(ket1,bra2);
						int extraSign = (s2i==1) ? FERMION_SIGN : 1;
						RealType cTemp = h*extraSign*basis_.doSign(
							ket1,ket2,i,orb,j,orb2,ProgramGlobals::SPIN_DOWN);
						sparseRow.add(temp,cTemp);
					}
				}
			}
		}
		
		void setU2OffDiagonalTerm(
				SparseRowType &sparseRow,
				const WordType& ket1,
				const WordType& ket2,
				SizeType i,
				SizeType orb1,
				const BasisType &basis) const
		{
			RealType val = FERMION_SIGN * mp_.hubbardU[2]*0.5;

			for (SizeType orb2=orb1+1;orb2<mp_.orbitals;orb2++) {
				setSplusSminus(sparseRow,ket1,ket2,i,orb1,i,orb2,val,basis);
				setSplusSminus(sparseRow,ket1,ket2,i,orb2,i,orb1,val,basis);
			}

		}

		// N.B.: orb1!=orb2 here
		void setSplusSminus(
				SparseRowType &sparseRow,
				const WordType& ket1,
				const WordType& ket2,
				SizeType i,
				SizeType orb1,
				SizeType j,
				SizeType orb2,
				RealType value,
				const BasisType &basis) const
		{
			if (splusSminusNonZero(ket1,ket2,i,orb1,j,orb2,basis)==0) return;

			SizeType ii = i*mp_.orbitals + orb1;
			SizeType jj = j*mp_.orbitals + orb2;
			assert(ii!=jj);
			WordType bra1 = ket1 ^ (BasisType::bitmask(ii)|BasisType::bitmask(jj));
			WordType bra2 = ket2 ^ (BasisType::bitmask(ii)|BasisType::bitmask(jj));
			SizeType temp = basis.perfectIndex(bra1,bra2);
			sparseRow.add(temp,value);
		}

		// N.B.: orb1!=orb2 here
		void setU3Term(
				SparseRowType &sparseRow,
				const WordType& ket1,
				const WordType& ket2,
				SizeType i,
				SizeType orb1,
				SizeType orb2,
				const BasisType &basis) const
		{
			assert(orb1!=orb2);
			if (u3TermNonZero(ket1,ket2,i,orb1,orb2,basis)==0) return;

			SizeType ii = i*mp_.orbitals + orb1;
			SizeType jj = i*mp_.orbitals + orb2;
			WordType bra1 = ket1 ^ (BasisType::bitmask(ii)|BasisType::bitmask(jj));
			WordType bra2 = ket2 ^ (BasisType::bitmask(ii)|BasisType::bitmask(jj));
			SizeType temp = basis.perfectIndex(bra1,bra2);
			sparseRow.add(temp,FERMION_SIGN * mp_.hubbardU[3]);
		}

		void setJTermOffDiagonal(
				SparseRowType &sparseRow,
				const WordType& ket1,
				const WordType& ket2,
				SizeType i,
				SizeType orb,
				const BasisType &basis) const
		{
			for (SizeType j=0;j<geometry_.numberOfSites();j++) {
				RealType value = jCoupling(i,j)*0.5;
				if (value==0) continue;
				value *= 0.5; // RealType counting i,j
				assert(i!=j);
				for (SizeType orb2=0;orb2<mp_.orbitals;orb2++) {
					//if (orb2!=orb) continue; // testing only!!
					int sign = jTermSign(ket1,ket2,i,orb,j,orb2,basis);
					setSplusSminus(sparseRow,ket1,ket2,
							i,orb,j,orb2,value*sign,basis);
					setSplusSminus(sparseRow,ket1,ket2,
							j,orb2,i,orb,value*sign,basis);
				}
			}
		}

		int jTermSign(
				const WordType& ket1,
				const WordType& ket2,
				SizeType i,
				SizeType orb1,
				SizeType j,
				SizeType orb2,
				const BasisType &basis) const
		{
			if (i>j) return jTermSign(ket1,ket2,j,orb2,i,orb1,basis);
			int x = basis.doSign(ket1,ket2,i,orb1,j,orb2,ProgramGlobals::SPIN_UP);
			x *= basis.doSign(ket1,ket2,i,orb1,j,orb2,ProgramGlobals::SPIN_DOWN);
			return x;
		}

		void calcDiagonalElements(typename PsimagLite::Vector<RealType>::Type& diag,
		                          const BasisType &basis) const
		{
			SizeType hilbert=basis.size();
			SizeType nsite = geometry_.numberOfSites();

			// Calculate diagonal elements
			for (SizeType ispace=0;ispace<hilbert;ispace++) {
				WordType ket1 = basis(ispace,ProgramGlobals::SPIN_UP);
				WordType ket2 = basis(ispace,ProgramGlobals::SPIN_DOWN);
				diag[ispace]=findS(nsite,ket1,ket2,ispace,basis);
			}
		}

		void calcDiagonalElements(VectorType &x,
					  const VectorType& y,
					  const BasisType* basis) const
		{
			SizeType hilbert=basis->size();
			SizeType nsite = geometry_.numberOfSites();

			for (SizeType ispace=0;ispace<hilbert;ispace++) {
				WordType ket1 = basis->operator()(ispace,ProgramGlobals::SPIN_UP);
				WordType ket2 = basis->operator()(ispace,ProgramGlobals::SPIN_DOWN);
				x[ispace] += findS(nsite,ket1,ket2,ispace,*basis)*y[ispace];
			}
		}

		RealType findS(SizeType nsite,WordType ket1,WordType ket2,SizeType ispace,const BasisType& basis) const
		{
			RealType s = 0;
			for (SizeType i=0;i<nsite;i++) {
				for (SizeType orb=0;orb<mp_.orbitals;orb++) {

					// Hubbard term U0
					s += mp_.hubbardU[0] * basis.isThereAnElectronAt(ket1,ket2,
											 i,ProgramGlobals::SPIN_UP,orb) * basis.isThereAnElectronAt(ket1,ket2,
																    i,ProgramGlobals::SPIN_DOWN,orb);


					for (SizeType orb2=orb+1;orb2<mp_.orbitals;orb2++) {
						// Hubbard term U1
						s += mp_.hubbardU[1] * nix(ket1,ket2,i,orb,basis) *
								nix(ket1,ket2,i,orb2,basis);

						// Diagonal U2 term
						s+= mp_.hubbardU[2]*
								szTerm(ket1,ket2,i,orb,basis)*
								szTerm(ket1,ket2,i,orb2,basis);
					}

					// JNN and JNNN diagonal part
					for (SizeType j=0;j<nsite;j++) {
						for (SizeType orb2=0;orb2<mp_.orbitals;orb2++) {
							RealType value = jCoupling(i,j);
							if (value==0) continue;
							s += value*0.5* // RealType counting i,j
									szTerm(ket1,ket2,i,orb,basis)*
									szTerm(ket1,ket2,j,orb2,basis);
						}
					}

					// Potential term
					s += mp_.potentialV[i+(orb+mp_.orbitals*0)*nsite]*
							basis.getN(ket1,i,ProgramGlobals::SPIN_UP,orb) +
						mp_.potentialV[i+(orb+mp_.orbitals*1)*nsite]*
							 basis.getN(ket2,i,ProgramGlobals::SPIN_DOWN,orb);

				}
			}
			return s;
		}

		SizeType splusSminusNonZero(
						const WordType& ket1,
						const WordType& ket2,
						SizeType i,
						SizeType orb1,
						SizeType j,
						SizeType orb2,
						const BasisType &basis) const
		{
			if (basis.isThereAnElectronAt(ket1,ket2,
					j,ProgramGlobals::SPIN_UP,orb2)==0) return 0;
			if (basis.isThereAnElectronAt(ket1,ket2,
					i,ProgramGlobals::SPIN_UP,orb1)==1) return 0;
			if (basis.isThereAnElectronAt(ket1,ket2,
					i,ProgramGlobals::SPIN_DOWN,orb1)==0) return 0;
			if (basis.isThereAnElectronAt(ket1,ket2,
					j,ProgramGlobals::SPIN_DOWN,orb2)==1) return 0;
			return 1;
		}

		SizeType u3TermNonZero(
				const WordType& ket1,
				const WordType& ket2,
				SizeType i,
				SizeType orb1,
				SizeType orb2,
				const BasisType &basis) const
		{
			if (basis.isThereAnElectronAt(ket1,ket2,
					i,ProgramGlobals::SPIN_UP,orb2)==0) return 0;
			if (basis.isThereAnElectronAt(ket1,ket2,
					i,ProgramGlobals::SPIN_UP,orb1)==1) return 0;
			if (basis.isThereAnElectronAt(ket1,ket2,
					i,ProgramGlobals::SPIN_DOWN,orb1)==1) return 0;
			if (basis.isThereAnElectronAt(ket1,ket2,
					i,ProgramGlobals::SPIN_DOWN,orb2)==0) return 0;
			return 1;
		}

		SizeType nix(
				const WordType& ket1,
				const WordType& ket2,
				SizeType i,
				SizeType orb,
				const BasisType &basis) const
		{
			SizeType sum = 0;
			for (SizeType spin=0;spin<2;spin++)
				sum += basis.isThereAnElectronAt(ket1,ket2,i,spin,orb);
			return sum;
		}

		RealType szTerm(
				const WordType& ket1,
				const WordType& ket2,
				SizeType i,
				SizeType orb,
				const BasisType &basis) const
		{
			RealType sz = basis.isThereAnElectronAt(ket1,ket2,i,ProgramGlobals::SPIN_UP,orb);
			sz -= basis.isThereAnElectronAt(ket1,ket2,i,ProgramGlobals::SPIN_DOWN,orb);
			return 0.5*sz;
		}
		
		RealType jCoupling(SizeType i,SizeType j) const
		{
			if (geometry_.terms()==1) return 0;
			return geometry_(i,0,j,0,TERM_J);
		}

		const ParametersModelType& mp_;
		const GeometryType& geometry_;
		BasisType basis_;
		
	}; // class FeBasedSc
	
} // namespace LanczosPlusPlus
#endif

