#ifndef LANCZOS_MODEL_SELECTOR_H
#define LANCZOS_MODEL_SELECTOR_H

#include "Vector.h"
#include "String.h"
#include "ModelBase.h"
#include "../Models/Tj1Orb/Tj1Orb.h"
#include "../Models/Immm/Immm.h"
#include "../Models/HubbardOneOrbital/HubbardOneOrbital.h"
#include "../Models/FeBasedSc/FeBasedSc.h"

namespace LanczosPlusPlus {

template<typename ComplexOrRealType,typename GeometryType,typename InputType>
class ModelSelector {

	typedef Tj1Orb<ComplexOrRealType,GeometryType, InputType> Tj1OrbType;
	typedef Immm<ComplexOrRealType,GeometryType, InputType> ImmmType;
	typedef HubbardOneOrbital<ComplexOrRealType,GeometryType, InputType> HubbardOneOrbitalType;
	typedef FeBasedSc<ComplexOrRealType,GeometryType, InputType> FeBasedScType;
	typedef typename PsimagLite::Real<ComplexOrRealType>::Type RealType;

public:

	typedef ModelBase<ComplexOrRealType,GeometryType, InputType> ModelBaseType;

	/** @class hide_modelselector
	 - TargetElectronsUp=integer
	 - TargetElectronsDown=integer
	 - Model=string One of Tj1Orb Heisenberg Immm
	                       HubbardOneBand HubbardOneBandExtended SuperHubbardExtended
	                       FeAsBasedSc FeAsBasedScExtended
	*/
	ModelSelector(InputType& io, const GeometryType& geometry)
	: modelPtr_(0)
	{
		SizeType nup = 0;
		SizeType ndown = 0;
		SizeType ntotal = 0;

		try {
			io.readline(nup,"TargetElectronsUp=");
			io.readline(ndown,"TargetElectronsDown=");
			ntotal = nup + ndown;
		} catch (std::exception& e) {
			io.read(ntotal,"TargetElectronsTotal=");
			io.read(nup,"TargetSzPlusConst=");

			assert(ntotal >= nup);
			ndown = ntotal - nup;
		}

		PsimagLite::String model("");
		io.readline(model,"Model=");

		if (model=="Tj1Orb" || model == "Heisenberg") {
			modelPtr_ = new Tj1OrbType(nup,ndown,io,geometry);
		} else if (model=="Immm") {
			modelPtr_ = new ImmmType(nup,ndown,io,geometry);
		} else if (model=="HubbardOneBand" ||
		           model=="HubbardOneBandExtended" ||
		           model=="SuperHubbardExtended" ||
		           model=="KaneMeleHubbard") {
			modelPtr_ = new HubbardOneOrbitalType(nup,ndown,io,geometry);
		} else if (model=="FeAsBasedSc" || model=="FeAsBasedScExtended") {
			modelPtr_ = new FeBasedScType(nup,ndown,io,geometry);
		} else {
			PsimagLite::String str("No known model " + model + "\n");
			throw PsimagLite::RuntimeError(str);
		}
	}

	~ModelSelector()
	{
		delete modelPtr_;
	}

	const ModelBaseType& operator()() const { return *modelPtr_; }

private:

	ModelBaseType* modelPtr_;

};

} // namespace LanczosPlusPlus

#endif

