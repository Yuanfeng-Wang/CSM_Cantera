/**
 *  @file InterfaceKinetics.cpp
 *
 */

// Copyright 2002  California Institute of Technology

#include "cantera/kinetics/InterfaceKinetics.h"
#include "cantera/kinetics/EdgeKinetics.h"
#include "cantera/thermo/SurfPhase.h"

#include "cantera/kinetics/ReactionData.h"
#include "cantera/kinetics/RateCoeffMgr.h"

#include "ImplicitSurfChem.h"

using namespace std;

namespace Cantera
{
//====================================================================================================================
InterfaceKineticsData::InterfaceKineticsData() :
    m_logp0(0.0),
    m_logc0(0.0),
    m_ROP_ok(false),
    m_temp(0.0),
    m_logtemp(0.0)
{
}
//====================================================================================================================
InterfaceKineticsData:: InterfaceKineticsData(const InterfaceKineticsData& right) :
    m_logp0(0.0),
    m_logc0(0.0),
    m_ROP_ok(false),
    m_temp(0.0),
    m_logtemp(0.0)
{
    *this = right;
}
//====================================================================================================================
InterfaceKineticsData::~InterfaceKineticsData()
{
}
//====================================================================================================================
InterfaceKineticsData& InterfaceKineticsData::operator=(const InterfaceKineticsData& right)
{
    if (this == &right) {
        return *this;
    }
    m_logp0 = right.m_logp0;
    m_logc0 = right.m_logc0;
    m_ropf = right.m_ropf;
    m_ropr = right.m_ropr;
    m_ropnet = right.m_ropnet;
    m_ROP_ok = right.m_ROP_ok;
    m_temp = right.m_temp;
    m_logtemp = right.m_logtemp;
    m_rfn = right.m_rfn;
    m_rkcn = right.m_rkcn;
    return *this;
}
//====================================================================================================================
/*
 * Construct an empty InterfaceKinetics reaction mechanism.
 *  @param thermo This is an optional parameter that may be
 *         used to initialize the inherited Kinetics class with
 *         one ThermoPhase class object -> in other words it's
 *         useful for initialization of homogeneous kinetics
 *         mechanisms.
 */
InterfaceKinetics::InterfaceKinetics(thermo_t* thermo) :
    Kinetics(),
    m_redo_rates(false),
    m_nirrev(0),
    m_nrev(0),
    m_surf(0),
    m_integrator(0),
    m_beta(0),
    m_ctrxn(0),
    m_ctrxn_ecdf(0),
    m_StandardConc(0),
    m_deltaG0(0),
    m_ProdStanConcReac(0),
    m_finalized(false),
    m_has_coverage_dependence(false),
    m_has_electrochem_rxns(false),
    m_has_exchange_current_density_formulation(false),
    m_phaseExistsCheck(false),
    m_phaseExists(0),
    m_phaseIsStable(0),
    m_rxnPhaseIsReactant(0),
    m_rxnPhaseIsProduct(0),
    m_ioFlag(0)
{
    if (thermo != 0) {
        addPhase(*thermo);
    }
    m_kdata = new InterfaceKineticsData;
    m_kdata->m_temp = 0.0;
}
//====================================================================================================================
/*
 * Destructor
 */
InterfaceKinetics::~InterfaceKinetics()
{
    delete m_kdata;
    if (m_integrator) {
        delete m_integrator;
    }
    for (size_t i = 0; i < m_ii; i++) {
        delete [] m_rxnPhaseIsReactant[i];
        delete [] m_rxnPhaseIsProduct[i];
    }
}
//====================================================================================================================
//  Copy Constructor for the %InterfaceKinetics object.
/*
 * Currently, this is not fully implemented. If called it will
 * throw an exception.
 */
InterfaceKinetics::InterfaceKinetics(const InterfaceKinetics& right) :
    Kinetics(),
    m_redo_rates(false),
    m_nirrev(0),
    m_nrev(0),
    m_surf(0),
    m_integrator(0),
    m_beta(0),
    m_ctrxn(0),
    m_ctrxn_ecdf(0),
    m_StandardConc(0),
    m_deltaG0(0),
    m_ProdStanConcReac(0),
    m_finalized(false),
    m_has_coverage_dependence(false),
    m_has_electrochem_rxns(false),
    m_has_exchange_current_density_formulation(false),
    m_phaseExistsCheck(false),
    m_phaseExists(0),
    m_phaseIsStable(0),
    m_rxnPhaseIsReactant(0),
    m_rxnPhaseIsProduct(0),
    m_ioFlag(0)
{
    m_kdata = new InterfaceKineticsData;
    m_kdata->m_temp = 0.0;
    /*
     * Call the assignment operator
     */
    *this = operator=(right);
}
//====================================================================================================================
// Assignment operator
/*
 *  This is NOT a virtual function.
 *
 * @param right    Reference to %Kinetics object to be copied into the
 *                 current one.
 */
InterfaceKinetics& InterfaceKinetics::
operator=(const InterfaceKinetics& right)
{
    /*
     * Check for self assignment.
     */
    if (this == &right) {
        return *this;
    }

    for (size_t i = 0; i < m_ii; i++) {
        delete [] m_rxnPhaseIsReactant[i];
        delete [] m_rxnPhaseIsProduct[i];
    }

    Kinetics::operator=(right);

    m_grt                  = right.m_grt;
    m_revindex             = right.m_revindex;
    m_rates                = right.m_rates;
    m_redo_rates           = right.m_redo_rates;
    m_index                = right.m_index;
    m_irrev                = right.m_irrev;
    m_rxnstoich            = right.m_rxnstoich;
    m_nirrev               = right.m_nirrev;
    m_nrev                 = right.m_nrev;
    m_rrxn                 = right.m_rrxn;
    m_prxn                 = right.m_prxn;
    m_rxneqn               = right.m_rxneqn;
    *m_kdata               = *right.m_kdata;
    m_conc                 = right.m_conc;
    m_mu0                  = right.m_mu0;
    m_phi                  = right.m_phi;
    m_pot                  = right.m_pot;
    m_rwork                = right.m_rwork;
    m_E                    = right.m_E;
    m_surf                 = right.m_surf;  //DANGER - shallow copy
    m_integrator           = right.m_integrator;  //DANGER - shallow copy
    m_beta                 = right.m_beta;
    m_ctrxn                = right.m_ctrxn;
    m_ctrxn_ecdf           = right.m_ctrxn_ecdf;
    m_StandardConc         = right.m_StandardConc;
    m_deltaG0              = right.m_deltaG0;
    m_ProdStanConcReac     = right.m_ProdStanConcReac;
    m_finalized            = right.m_finalized;
    m_has_coverage_dependence = right.m_has_coverage_dependence;
    m_has_electrochem_rxns = right.m_has_electrochem_rxns;
    m_has_exchange_current_density_formulation = right.m_has_exchange_current_density_formulation;
    m_phaseExistsCheck     = right.m_phaseExistsCheck;
    m_phaseExists          = right.m_phaseExists;
    m_phaseIsStable        = right.m_phaseIsStable;

    m_rxnPhaseIsReactant.resize(m_ii, 0);
    m_rxnPhaseIsProduct.resize(m_ii, 0);
    size_t np = nPhases();
    for (size_t i = 0; i < m_ii; i++) {
        m_rxnPhaseIsReactant[i] = new bool[np];
        m_rxnPhaseIsProduct[i] = new bool[np];
        for (size_t p = 0; p < np; p++) {
            m_rxnPhaseIsReactant[i][p] = right.m_rxnPhaseIsReactant[i][p];
            m_rxnPhaseIsProduct[i][p] = right.m_rxnPhaseIsProduct[i][p];
        }
    }

    m_ioFlag               = right.m_ioFlag;

    return *this;
}
//====================================================================================================================
// Return the ID of the kinetics object
int  InterfaceKinetics::ID() const
{
    return cInterfaceKinetics;
}
//====================================================================================================================
int InterfaceKinetics::type() const
{
    return cInterfaceKinetics;
}
//====================================================================================================================
// Duplication routine for objects which inherit from Kinetics
/*
 *  This virtual routine can be used to duplicate %Kinetics objects
 *  inherited from %Kinetics even if the application only has
 *  a pointer to %Kinetics to work with.
 *
 *  These routines are basically wrappers around the derived copy
 *  constructor.
 *
 * @param  tpVector Vector of shallow pointers to ThermoPhase objects. this is the
 *                  m_thermo vector within this object
 */
Kinetics* InterfaceKinetics::duplMyselfAsKinetics(const std::vector<thermo_t*> & tpVector) const
{
    InterfaceKinetics* iK = new InterfaceKinetics(*this);
    iK->assignShallowPointers(tpVector);
    return dynamic_cast<Kinetics*>(iK);
}
//====================================================================================================================
// Set the electric potential in the nth phase
/*
 * @param n phase Index in this kinetics object.
 * @param V Electric potential (volts)
 */
void InterfaceKinetics::setElectricPotential(int n, doublereal V)
{
    thermo(n).setElectricPotential(V);
    m_redo_rates = true;
}
//====================================================================================================================
// Update properties that depend on temperature
/*
 *  This is called to update all of the properties that depend on temperature
 *
 *  Current objects that this function updates
 *       m_kdata->m_logtemp
 *       m_kdata->m_rfn
 *       m_rates.
 *       updateKc();
 */
void InterfaceKinetics::_update_rates_T()
{
    _update_rates_phi();
    if (m_has_coverage_dependence) {
        m_surf->getCoverages(DATA_PTR(m_conc));
        m_rates.update_C(DATA_PTR(m_conc));
        m_redo_rates = true;
    }
    doublereal T = thermo(surfacePhaseIndex()).temperature();
    m_redo_rates = true;
    if (T != m_kdata->m_temp || m_redo_rates) {
        m_kdata->m_logtemp = log(T);
        m_rates.update(T, m_kdata->m_logtemp, DATA_PTR(m_kdata->m_rfn));
        if (m_has_exchange_current_density_formulation) {
            applyExchangeCurrentDensityFormulation(DATA_PTR(m_kdata->m_rfn));
        }
        if (m_has_electrochem_rxns) {
            applyButlerVolmerCorrection(DATA_PTR(m_kdata->m_rfn));
        }
        m_kdata->m_temp = T;
        updateKc();
        m_kdata->m_ROP_ok = false;
        m_redo_rates = false;
    }
}
//====================================================================================================================
void InterfaceKinetics::_update_rates_phi()
{
    for (size_t n = 0; n < nPhases(); n++) {
        if (thermo(n).electricPotential() != m_phi[n]) {
            m_phi[n] = thermo(n).electricPotential();
            m_redo_rates = true;
        }
    }
}
//====================================================================================================================


/**
 * Update properties that depend on concentrations. This method
 * fills out the array of generalized concentrations by calling
 * method getActivityConcentrations for each phase, which classes
 * representing phases should overload to return the appropriate
 * quantities.
 */
void InterfaceKinetics::_update_rates_C()
{
    for (size_t n = 0; n < nPhases(); n++) {
        /*
         * We call the getActivityConcentrations function of each
         * ThermoPhase class that makes up this kinetics object to
         * obtain the generalized concentrations for species within that
         * class. This is collected in the vector m_conc. m_start[]
         * are integer indices for that vector denoting the start of the
         * species for each phase.
         */
        thermo(n).getActivityConcentrations(DATA_PTR(m_conc) + m_start[n]);
    }
    m_kdata->m_ROP_ok = false;
}


// Get the vector of activity concentrations used in the kinetics object
/*
 *  @param conc  (output) Vector of activity concentrations. Length is
 *               equal to the number of species in the kinetics object
 */
void InterfaceKinetics::getActivityConcentrations(doublereal* const conc)
{
    _update_rates_C();
    copy(m_conc.begin(), m_conc.end(), conc);
}


/**
 * Update the equilibrium constants in molar units for all
 * reversible reactions. Irreversible reactions have their
 * equilibrium constant set to zero.
 */
void InterfaceKinetics::updateKc()
{
    vector_fp& m_rkc = m_kdata->m_rkcn;
    fill(m_rkc.begin(), m_rkc.end(), 0.0);

    //static vector_fp mu(nTotalSpecies());
    if (m_nrev > 0) {

        size_t nsp, ik = 0;
        doublereal rt = GasConstant*thermo(0).temperature();
        doublereal rrt = 1.0 / rt;
        size_t np = nPhases();
        for (size_t n = 0; n < np; n++) {
            thermo(n).getStandardChemPotentials(DATA_PTR(m_mu0) + m_start[n]);
            nsp = thermo(n).nSpecies();
            for (size_t k = 0; k < nsp; k++) {
                m_mu0[ik] -= rt * thermo(n).logStandardConc(k);
                m_mu0[ik] += Faraday * m_phi[n] * thermo(n).charge(k);
                ik++;
            }
        }

        // compute Delta mu^0 for all reversible reactions
        m_rxnstoich.getRevReactionDelta(m_ii, DATA_PTR(m_mu0),
                                        DATA_PTR(m_rkc));

        for (size_t i = 0; i < m_nrev; i++) {
            size_t irxn = m_revindex[i];
            if (irxn == npos || irxn >= nReactions()) {
                throw CanteraError("InterfaceKinetics",
                                   "illegal value: irxn = "+int2str(irxn));
            }
            m_rkc[irxn] = exp(m_rkc[irxn]*rrt);
        }
        for (size_t i = 0; i != m_nirrev; ++i) {
            m_rkc[ m_irrev[i] ] = 0.0;
        }
    }
}
//====================================================================================================================



void InterfaceKinetics::checkPartialEquil()
{
    vector_fp dmu(nTotalSpecies(), 0.0);
    vector_fp frop(std::max<size_t>(nReactions(), 1), 0.0);
    vector_fp rrop(std::max<size_t>(nReactions(), 1), 0.0);
    vector_fp netrop(std::max<size_t>(nReactions(), 1), 0.0);
    vector_fp rmu(std::max<size_t>(nReactions(), 1), 0.0);
    if (m_nrev > 0) {
        doublereal rt = GasConstant*thermo(0).temperature();
        cout << "T = " << thermo(0).temperature() << " " << rt << endl;
        size_t nsp, ik=0;
        //doublereal rt = GasConstant*thermo(0).temperature();
        //            doublereal rrt = 1.0/rt;
        doublereal delta;
        for (size_t n = 0; n < nPhases(); n++) {
            thermo(n).getChemPotentials(DATA_PTR(dmu) + m_start[n]);
            nsp = thermo(n).nSpecies();
            for (size_t k = 0; k < nsp; k++) {
                delta = Faraday * m_phi[n] * thermo(n).charge(k);
                //cout << thermo(n).speciesName(k) << "   " << (delta+dmu[ik])/rt << " " << dmu[ik]/rt << endl;
                dmu[ik] += delta;
                ik++;
            }
        }

        // compute Delta mu^ for all reversible reactions
        m_rxnstoich.getRevReactionDelta(m_ii, DATA_PTR(dmu), DATA_PTR(rmu));
        getFwdRatesOfProgress(DATA_PTR(frop));
        getRevRatesOfProgress(DATA_PTR(rrop));
        getNetRatesOfProgress(DATA_PTR(netrop));
        for (size_t i = 0; i < m_nrev; i++) {
            size_t irxn = m_revindex[i];
            cout << "Reaction " << reactionString(irxn)
                 << "  " << rmu[irxn]/rt << endl;
            printf("%12.6e  %12.6e  %12.6e  %12.6e \n",
                   frop[irxn], rrop[irxn], netrop[irxn],
                   netrop[irxn]/(frop[irxn] + rrop[irxn]));
        }
    }
}


/**
 * Get the equilibrium constants of all reactions, whether
 * reversible or not.
 */
void InterfaceKinetics::getEquilibriumConstants(doublereal* kc)
{
    size_t ik=0;
    doublereal rt = GasConstant*thermo(0).temperature();
    doublereal rrt = 1.0/rt;
    for (size_t n = 0; n < nPhases(); n++) {
        thermo(n).getStandardChemPotentials(DATA_PTR(m_mu0) + m_start[n]);
        size_t nsp = thermo(n).nSpecies();
        for (size_t k = 0; k < nsp; k++) {
            m_mu0[ik] -= rt*thermo(n).logStandardConc(k);
            m_mu0[ik] += Faraday * m_phi[n] * thermo(n).charge(k);
            ik++;
        }
    }

    fill(kc, kc + m_ii, 0.0);

    m_rxnstoich.getReactionDelta(m_ii, DATA_PTR(m_mu0), kc);

    for (size_t i = 0; i < m_ii; i++) {
        kc[i] = exp(-kc[i]*rrt);
    }
}

void InterfaceKinetics::getExchangeCurrentQuantities()
{
    /*
     * First collect vectors of the standard Gibbs free energies of the
     * species and the standard concentrations
     *   - m_mu0
     *   - m_logStandardConc
     */
    size_t ik = 0;

    for (size_t n = 0; n < nPhases(); n++) {
        thermo(n).getStandardChemPotentials(DATA_PTR(m_mu0) + m_start[n]);
        size_t nsp = thermo(n).nSpecies();
        for (size_t k = 0; k < nsp; k++) {
            m_StandardConc[ik] = thermo(n).standardConcentration(k);
            ik++;
        }
    }

    m_rxnstoich.getReactionDelta(m_ii, DATA_PTR(m_mu0), DATA_PTR(m_deltaG0));

    for (size_t i = 0; i < m_ii; i++) {
        m_ProdStanConcReac[i] = 1.0;
    }

    m_rxnstoich.multiplyReactants(DATA_PTR(m_StandardConc), DATA_PTR(m_ProdStanConcReac));

}

// Returns the Species creation rates [kmol/m^2/s].
/*
 *   Return the species
 * creation rates in array cdot, which must be
 * dimensioned at least as large as the total number of
 * species in all phases of the kinetics
 * model
 *
 *  @param cdot Vector containing the creation rates.
 *              length = m_kk. units = kmol/m^2/s
 */
void InterfaceKinetics::getCreationRates(doublereal* cdot)
{
    updateROP();
    m_rxnstoich.getCreationRates(m_kk, &m_kdata->m_ropf[0],
                                 &m_kdata->m_ropr[0], cdot);
}

//  Return the Species destruction rates [kmol/m^2/s].
/*
 *  Return the species destruction rates in array ddot, which must be
 *  dimensioned at least as large as the total number of
 *  species in all phases of the kinetics model
 */
void InterfaceKinetics::getDestructionRates(doublereal* ddot)
{
    updateROP();
    m_rxnstoich.getDestructionRates(m_kk, &m_kdata->m_ropf[0],
                                    &m_kdata->m_ropr[0], ddot);
}

// Return the species net production rates
/*
 * Species net production rates [kmol/m^2/s]. Return the species
 * net production rates (creation - destruction) in array
 * wdot, which must be dimensioned at least as large as the
 * total number of species in all phases of the kinetics
 * model
 *
 * @param net  Vector of species production rates.
 *             units kmol m-d s-1, where d is dimension.
 */
void InterfaceKinetics::getNetProductionRates(doublereal* net)
{
    updateROP();
    m_rxnstoich.getNetProductionRates(m_kk,
                                      &m_kdata->m_ropnet[0],
                                      net);
}

//====================================================================================================================
// Apply corrections for interfacial charge transfer reactions
/*
 * For reactions that transfer charge across a potential difference,
 * the activation energies are modified by the potential difference.
 * (see, for example, ...). This method applies this correction.
 *
 * @param kf  Vector of forward reaction rate constants on which to have
 *            the correction applied
 */
void InterfaceKinetics::applyButlerVolmerCorrection(doublereal* const kf)
{
    // compute the electrical potential energy of each species
    size_t ik = 0;
    for (size_t n = 0; n < nPhases(); n++) {
        size_t nsp = thermo(n).nSpecies();
        for (size_t k = 0; k < nsp; k++) {
            m_pot[ik] = Faraday*thermo(n).charge(k)*m_phi[n];
            ik++;
        }
    }

    // Compute the change in electrical potential energy for each
    // reaction. This will only be non-zero if a potential
    // difference is present.
    m_rxnstoich.getReactionDelta(m_ii, DATA_PTR(m_pot), DATA_PTR(m_rwork));

    // Modify the reaction rates. Only modify those with a
    // non-zero activation energy. Below we decrease the
    // activation energy below zero but in some debug modes
    // we print out a warning message about this.
    /*
     *   NOTE, there is some discussion about this point.
     *   Should we decrease the activation energy below zero?
     *   I don't think this has been decided in any definitive way.
     *   The treatment below is numerically more stable, however.
     */
    doublereal eamod;
#ifdef DEBUG_KIN_MODE
    doublereal ea;
#endif
    for (size_t i = 0; i < m_beta.size(); i++) {
        size_t irxn = m_ctrxn[i];
        eamod = m_beta[i]*m_rwork[irxn];
        //  if (eamod != 0.0 && m_E[irxn] != 0.0) {
        if (eamod != 0.0) {
#ifdef DEBUG_KIN_MODE
            ea = GasConstant * m_E[irxn];
            if (eamod + ea < 0.0) {
                writelog("Warning: act energy mod too large!\n");
                writelog("  Delta phi = "+fp2str(m_rwork[irxn]/Faraday)+"\n");
                writelog("  Delta Ea = "+fp2str(eamod)+"\n");
                writelog("  Ea = "+fp2str(ea)+"\n");
                for (n = 0; n < np; n++) {
                    writelog("Phase "+int2str(n)+": phi = "
                             +fp2str(m_phi[n])+"\n");
                }
            }
#endif
            doublereal rt = GasConstant*thermo(0).temperature();
            doublereal rrt = 1.0/rt;
            kf[irxn] *= exp(-eamod*rrt);
        }
    }
}
//====================================================================================================================
void InterfaceKinetics::applyExchangeCurrentDensityFormulation(doublereal* const kfwd)
{
    getExchangeCurrentQuantities();
    doublereal rt = GasConstant*thermo(0).temperature();
    doublereal rrt = 1.0/rt;
    for (size_t i = 0; i < m_ctrxn.size(); i++) {
        size_t irxn = m_ctrxn[i];
        int iECDFormulation =  m_ctrxn_ecdf[i];
        if (iECDFormulation) {
            double tmp = exp(- m_beta[i] * m_deltaG0[irxn] * rrt);
            double tmp2 = m_ProdStanConcReac[irxn];
            tmp *= 1.0  / tmp2 / Faraday;
            kfwd[irxn] *= tmp;
        }
    }


}
//====================================================================================================================
/**
 * Update the rates of progress of the reactions in the reaction
 * mechanism. This routine operates on internal data.
 */
void InterfaceKinetics::getFwdRateConstants(doublereal* kfwd)
{

    updateROP();

    const vector_fp& rf = m_kdata->m_rfn;

    // copy rate coefficients into kfwd
    copy(rf.begin(), rf.end(), kfwd);

    // multiply by perturbation factor
    multiply_each(kfwd, kfwd + nReactions(), m_perturb.begin());

}
//====================================================================================================================

/**
 * Update the rates of progress of the reactions in the reaction
 * mechanism. This routine operates on internal data.
 */
void InterfaceKinetics::getRevRateConstants(doublereal* krev, bool doIrreversible)
{
    getFwdRateConstants(krev);
    if (doIrreversible) {
        doublereal* tmpKc = DATA_PTR(m_kdata->m_ropnet);
        getEquilibriumConstants(tmpKc);
        for (size_t i = 0; i < m_ii; i++) {
            krev[i] /=  tmpKc[i];
        }
    } else {
        const vector_fp& rkc = m_kdata->m_rkcn;
        multiply_each(krev, krev + nReactions(), rkc.begin());
    }
}
//====================================================================================================================

void InterfaceKinetics::getActivationEnergies(doublereal* E)
{
    copy(m_E.begin(), m_E.end(), E);
}
//====================================================================================================================
/**
 * Update the rates of progress of the reactions in the reaction
 * mechanism. This routine operates on internal data.
 */
void InterfaceKinetics::updateROP()
{

    _update_rates_T();
    _update_rates_C();

    if (m_kdata->m_ROP_ok) {
        return;
    }

    const vector_fp& rf = m_kdata->m_rfn;
    const vector_fp& m_rkc = m_kdata->m_rkcn;
    vector_fp& ropf = m_kdata->m_ropf;
    vector_fp& ropr = m_kdata->m_ropr;
    vector_fp& ropnet = m_kdata->m_ropnet;

    // copy rate coefficients into ropf
    copy(rf.begin(), rf.end(), ropf.begin());

    // multiply by perturbation factor
    multiply_each(ropf.begin(), ropf.end(), m_perturb.begin());

    // copy the forward rates to the reverse rates
    copy(ropf.begin(), ropf.end(), ropr.begin());

    // for reverse rates computed from thermochemistry, multiply
    // the forward rates copied into m_ropr by the reciprocals of
    // the equilibrium constants
    multiply_each(ropr.begin(), ropr.end(), m_rkc.begin());

    // multiply ropf by concentration products
    m_rxnstoich.multiplyReactants(DATA_PTR(m_conc), DATA_PTR(ropf));
    //m_reactantStoich.multiply(m_conc.begin(), ropf.begin());

    // for reversible reactions, multiply ropr by concentration
    // products
    m_rxnstoich.multiplyRevProducts(DATA_PTR(m_conc),
                                    DATA_PTR(ropr));
    //m_revProductStoich.multiply(m_conc.begin(), ropr.begin());

    // do global reactions
    //m_globalReactantStoich.power(m_conc.begin(), ropf.begin());

    for (size_t j = 0; j != m_ii; ++j) {
        ropnet[j] = ropf[j] - ropr[j];
    }


    /*
     *  For reactions involving multiple phases, we must check that the phase
     *  being consumed actually exists. This is particularly important for
     *  phases that are stoichiometric phases containing one species with a unity activity
     */
    if (m_phaseExistsCheck) {
        for (size_t j = 0; j != m_ii; ++j) {
            if ((ropr[j] >  ropf[j]) && (ropr[j] > 0.0)) {
                for (size_t p = 0; p < nPhases(); p++) {
                    if (m_rxnPhaseIsProduct[j][p]) {
                        if (! m_phaseExists[p]) {
                            ropnet[j] = 0.0;
                            ropr[j] = ropf[j];
                            if (ropf[j] > 0.0) {
                                for (size_t rp = 0; rp < nPhases(); rp++) {
                                    if (m_rxnPhaseIsReactant[j][rp]) {
                                        if (! m_phaseExists[rp]) {
                                            ropnet[j] = 0.0;
                                            ropr[j] = ropf[j] = 0.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (m_rxnPhaseIsReactant[j][p]) {
                        if (! m_phaseIsStable[p]) {
                            ropnet[j] = 0.0;
                            ropr[j] = ropf[j];
                        }
                    }
                }
            } else if ((ropf[j] > ropr[j]) && (ropf[j] > 0.0)) {
                for (size_t p = 0; p < nPhases(); p++) {
                    if (m_rxnPhaseIsReactant[j][p]) {
                        if (! m_phaseExists[p]) {
                            ropnet[j] = 0.0;
                            ropf[j] = ropr[j];
                            if (ropf[j] > 0.0) {
                                for (size_t rp = 0; rp < nPhases(); rp++) {
                                    if (m_rxnPhaseIsProduct[j][rp]) {
                                        if (! m_phaseExists[rp]) {
                                            ropnet[j] = 0.0;
                                            ropf[j] = ropr[j] = 0.0;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (m_rxnPhaseIsProduct[j][p]) {
                        if (! m_phaseIsStable[p]) {
                            ropnet[j] = 0.0;
                            ropf[j] = ropr[j];
                        }
                    }
                }
            }
        }
    }

    m_kdata->m_ROP_ok = true;
}

#ifdef KINETICS_WITH_INTERMEDIATE_ZEROED_PHASES
//=================================================================================================
InterfaceKinetics::adjustRatesForIntermediatePhases()
{
    doublereal sFac = 1.0;

    vector_fp& ropf = m_kdata->m_ropf;
    vector_fp& ropr = m_kdata->m_ropr;
    vector_fp& ropnet = m_kdata->m_ropnet;

    getCreatingRates(DATA_PTR(m_speciestmpP));
    getDestructionRates(DATA_PTR(m_speciestmpD));

    for (iphase = 0; iphase < nphases; iphase++) {
        if (m_intermediatePhases(iphase)) {
            for (isp = 0; isp < nspecies; isp++) {
                if (m_speciesTmpD[ispI] > m_speciesTmpP[I]) {
                    sFac = m_speciesTmpD[ispI]/ m_speciesTmpP[I];
                }
                // Loop over reactions that are reactants for the species in the phase
                // reducing their rates.


            }
        }

    }

}
#endif
//=================================================================================================
//=================================================================================================
/*
 *
 * getDeltaGibbs():
 *
 * Return the vector of values for the reaction gibbs free energy
 * change
 * These values depend upon the concentration
 * of the ideal gas.
 *
 *  units = J kmol-1
 */
void InterfaceKinetics::getDeltaGibbs(doublereal* deltaG)
{
    /*
     * Get the chemical potentials of the species in the
     * ideal gas solution.
     */
    for (size_t n = 0; n < nPhases(); n++) {
        thermo(n).getChemPotentials(DATA_PTR(m_grt) + m_start[n]);
    }
    //for (n = 0; n < m_grt.size(); n++) {
    //    cout << n << "G_RT = " << m_grt[n] << endl;
    //}

    /*
     * Use the stoichiometric manager to find deltaG for each
     * reaction.
     */
    m_rxnstoich.getReactionDelta(m_ii, DATA_PTR(m_grt), deltaG);
}
//=================================================================================================
// Return the vector of values for the reaction electrochemical free energy change.
/*
 * These values depend upon the concentration of the solution and
 * the voltage of the phases
 *
 *  units = J kmol-1
 *
 * @param deltaM  Output vector of  deltaM's for reactions
 *                Length: m_ii.
 */
void InterfaceKinetics::getDeltaElectrochemPotentials(doublereal* deltaM)
{
    /*
     * Get the chemical potentials of the species in the
     * ideal gas solution.
     */
    size_t np = nPhases();
    for (size_t n = 0; n < np; n++) {
        thermo(n).getElectrochemPotentials(DATA_PTR(m_grt) + m_start[n]);
    }
    /*
     * Use the stoichiometric manager to find deltaG for each
     * reaction.
     */
    m_rxnstoich.getReactionDelta(m_ii, DATA_PTR(m_grt), deltaM);
}
//=================================================================================================
/*
 *
 * getDeltaEnthalpy():
 *
 * Return the vector of values for the reactions change in
 * enthalpy.
 * These values depend upon the concentration
 * of the solution.
 *
 *  units = J kmol-1
 */
void InterfaceKinetics::getDeltaEnthalpy(doublereal* deltaH)
{
    /*
     * Get the partial molar enthalpy of all species in the
     * ideal gas.
     */
    for (size_t n = 0; n < nPhases(); n++) {
        thermo(n).getPartialMolarEnthalpies(DATA_PTR(m_grt) + m_start[n]);
    }
    /*
     * Use the stoichiometric manager to find deltaG for each
     * reaction.
     */
    m_rxnstoich.getReactionDelta(m_ii, DATA_PTR(m_grt), deltaH);
}


// Return the vector of values for the change in
// entropy due to each reaction
/*
 * These values depend upon the concentration
 * of the solution.
 *
 *  units = J kmol-1 Kelvin-1
 *
 * @param deltaS vector of Enthalpy changes
 *        Length = m_ii, number of reactions
 *
 */
void InterfaceKinetics::getDeltaEntropy(doublereal* deltaS)
{
    /*
     * Get the partial molar entropy of all species in all of
     * the phases
     */
    for (size_t n = 0; n < nPhases(); n++) {
        thermo(n).getPartialMolarEntropies(DATA_PTR(m_grt) + m_start[n]);
    }
    /*
     * Use the stoichiometric manager to find deltaS for each
     * reaction.
     */
    m_rxnstoich.getReactionDelta(m_ii, DATA_PTR(m_grt), deltaS);
}

/**
 *
 * getDeltaSSGibbs():
 *
 * Return the vector of values for the reaction
 * standard state gibbs free energy change.
 * These values don't depend upon the concentration
 * of the solution.
 *
 *  units = J kmol-1
 */
void InterfaceKinetics::getDeltaSSGibbs(doublereal* deltaG)
{
    /*
     *  Get the standard state chemical potentials of the species.
     *  This is the array of chemical potentials at unit activity
     *  We define these here as the chemical potentials of the pure
     *  species at the temperature and pressure of the solution.
     */
    for (size_t n = 0; n < nPhases(); n++) {
        thermo(n).getStandardChemPotentials(DATA_PTR(m_grt) + m_start[n]);
    }
    /*
     * Use the stoichiometric manager to find deltaG for each
     * reaction.
     */
    m_rxnstoich.getReactionDelta(m_ii, DATA_PTR(m_grt), deltaG);
}

/**
 *
 * getDeltaSSEnthalpy():
 *
 * Return the vector of values for the change in the
 * standard state enthalpies of reaction.
 * These values don't depend upon the concentration
 * of the solution.
 *
 *  units = J kmol-1
 */
void InterfaceKinetics::getDeltaSSEnthalpy(doublereal* deltaH)
{
    /*
     *  Get the standard state enthalpies of the species.
     *  This is the array of chemical potentials at unit activity
     *  We define these here as the enthalpies of the pure
     *  species at the temperature and pressure of the solution.
     */
    for (size_t n = 0; n < nPhases(); n++) {
        thermo(n).getEnthalpy_RT(DATA_PTR(m_grt) + m_start[n]);
    }
    doublereal RT = thermo().temperature() * GasConstant;
    for (size_t k = 0; k < m_kk; k++) {
        m_grt[k] *= RT;
    }
    /*
     * Use the stoichiometric manager to find deltaG for each
     * reaction.
     */
    m_rxnstoich.getReactionDelta(m_ii, DATA_PTR(m_grt), deltaH);
}

/*********************************************************************
 *
 * getDeltaSSEntropy():
 *
 * Return the vector of values for the change in the
 * standard state entropies for each reaction.
 * These values don't depend upon the concentration
 * of the solution.
 *
 *  units = J kmol-1 Kelvin-1
 */
void InterfaceKinetics::getDeltaSSEntropy(doublereal* deltaS)
{
    /*
     *  Get the standard state entropy of the species.
     *  We define these here as the entropies of the pure
     *  species at the temperature and pressure of the solution.
     */
    for (size_t n = 0; n < nPhases(); n++) {
        thermo(n).getEntropy_R(DATA_PTR(m_grt) + m_start[n]);
    }
    doublereal R = GasConstant;
    for (size_t k = 0; k < m_kk; k++) {
        m_grt[k] *= R;
    }
    /*
     * Use the stoichiometric manager to find deltaS for each
     * reaction.
     */
    m_rxnstoich.getReactionDelta(m_ii, DATA_PTR(m_grt), deltaS);
}

//====================================================================================================================
/**
 * Add a single reaction to the mechanism. This routine
 * must be called after init() and before finalize().
 * This function branches on the types of reactions allowed
 * by the interfaceKinetics manager in order to install
 * the reaction correctly in the manager.
 * The manager allows the following reaction types
 *  Elementary
 *  Surface
 *  Global
 * There is no difference between elementary and surface
 * reactions.
 */
void InterfaceKinetics::addReaction(ReactionData& r)
{

    /*
     * Install the rate coefficient for the current reaction
     * in the appropriate data structure.
     */
    addElementaryReaction(r);
    /*
     * Add the reactants and products for  m_ropnet;the current reaction
     * to the various stoichiometric coefficient arrays.
     */
    installReagents(r);
    /*
     * Save the reaction and product groups, which are
     * part of the ReactionData class, in this class.
     * They aren't used for anything but reaction path
     * analysis.
     */
    //installGroups(reactionNumber(), r.rgroups, r.pgroups);
    /*
     * Increase the internal number of reactions, m_ii, by one.
     * increase the size of m_perturb by one as well.
     */
    incrementRxnCount();
    m_rxneqn.push_back(r.equation);

    m_rxnPhaseIsReactant.resize(m_ii, 0);
    m_rxnPhaseIsProduct.resize(m_ii, 0);

    size_t np = nPhases();
    size_t i = m_ii - 1;
    m_rxnPhaseIsReactant[i] = new bool[np];
    m_rxnPhaseIsProduct[i] = new bool[np];

    for (size_t p = 0; p < np; p++) {
        m_rxnPhaseIsReactant[i][p] = false;
        m_rxnPhaseIsProduct[i][p] = false;
    }

    const std::vector<size_t>& vr = reactants(i);
    for (size_t ik = 0; ik < vr.size(); ik++) {
        size_t k = vr[ik];
        size_t p = speciesPhaseIndex(k);
        m_rxnPhaseIsReactant[i][p] = true;
    }
    const std::vector<size_t>& vp = products(i);
    for (size_t ik = 0; ik < vp.size(); ik++) {
        size_t k = vp[ik];
        size_t p = speciesPhaseIndex(k);
        m_rxnPhaseIsProduct[i][p] = true;
    }
}
//====================================================================================================================
void InterfaceKinetics::addElementaryReaction(ReactionData& r)
{
    // install rate coeff calculator
    vector_fp& rp = r.rateCoeffParameters;
    size_t ncov = r.cov.size();
    if (ncov > 3) {
        m_has_coverage_dependence = true;
    }
    for (size_t m = 0; m < ncov; m++) {
        rp.push_back(r.cov[m]);
    }

    /*
     * Temporarily change the reaction rate coefficient type to surface arrhenius.
     * This is what is expected. We'll handle exchange current types below by hand.
     */
    int reactionRateCoeffType_orig = r.rateCoeffType;
    if (r.rateCoeffType == EXCHANGE_CURRENT_REACTION_RATECOEFF_TYPE) {
       r.rateCoeffType = SURF_ARRHENIUS_REACTION_RATECOEFF_TYPE;
    }
    if (r.rateCoeffType == ARRHENIUS_REACTION_RATECOEFF_TYPE) {
       r.rateCoeffType = SURF_ARRHENIUS_REACTION_RATECOEFF_TYPE;
    }
    /*
     * Install the reaction rate into the vector of reactions handled by this class
     */
    size_t iloc = m_rates.install(reactionNumber(), r);

    /*
     * Change the reaction rate coefficient type back to its original value
     */
    r.rateCoeffType = reactionRateCoeffType_orig;

    // store activation energy
    m_E.push_back(r.rateCoeffParameters[2]);

    if (r.beta > 0.0) {
        m_has_electrochem_rxns = true;
        m_beta.push_back(r.beta);
        m_ctrxn.push_back(reactionNumber());
        if (r.rateCoeffType == EXCHANGE_CURRENT_REACTION_RATECOEFF_TYPE) {
            m_has_exchange_current_density_formulation = true;
            m_ctrxn_ecdf.push_back(1);
        } else {
            m_ctrxn_ecdf.push_back(0);
        }
    }

    // add constant term to rate coeff value vector
    m_kdata->m_rfn.push_back(r.rateCoeffParameters[0]);
    registerReaction(reactionNumber(), ELEMENTARY_RXN, iloc);
}
//====================================================================================================================

void InterfaceKinetics::setIOFlag(int ioFlag)
{
    m_ioFlag = ioFlag;
    if (m_integrator) {
        m_integrator->setIOFlag(ioFlag);
    }
}

//     void InterfaceKinetics::
//     addGlobalReaction(const ReactionData& r) {

//         int iloc;
//         // install rate coeff calculator
//         vector_fp rp = r.rateCoeffParameters;
//         int ncov = r.cov.size();
//         for (int m = 0; m < ncov; m++) rp.push_back(r.cov[m]);
//         iloc = m_rates.install( reactionNumber(),
//             r.rateCoeffType, rp.size(),
//             rp.begin() );
//         // store activation energy
//         m_E.push_back(r.rateCoeffParameters[2]);
//         // add constant term to rate coeff value vector
//         m_kdata->m_rfn.push_back(r.rateCoeffParameters[0]);

//         int nr = r.order.size();
//         vector_fp ordr(nr);
//         for (int n = 0; n < nr; n++) {
//             ordr[n] = r.order[n] - r.rstoich[n];
//         }
//         m_globalReactantStoich.add( reactionNumber(),
//             r.reactants, ordr);

//         registerReaction( reactionNumber(), GLOBAL_RXN, iloc);
//     }


void InterfaceKinetics::installReagents(const ReactionData& r)
{

    size_t n, ns, m;
    doublereal nsFlt;
    /*
     * extend temporary storage by one for this rxn.
     */
    m_kdata->m_ropf.push_back(0.0);
    m_kdata->m_ropr.push_back(0.0);
    m_kdata->m_ropnet.push_back(0.0);
    m_kdata->m_rkcn.push_back(0.0);

    /*
     * Obtain the current reaction index for the reaction that we
     * are adding. The first reaction is labeled 0.
     */
    size_t rnum = reactionNumber();

    // vectors rk and pk are lists of species numbers, with
    // repeated entries for species with stoichiometric
    // coefficients > 1. This allows the reaction to be defined
    // with unity reaction order for each reactant, and so the
    // faster method 'multiply' can be used to compute the rate of
    // progress instead of 'power'.

    std::vector<size_t> rk;
    size_t nr = r.reactants.size();
    for (n = 0; n < nr; n++) {
        nsFlt = r.rstoich[n];
        ns = (size_t) nsFlt;
        if ((doublereal) ns != nsFlt) {
            if (ns < 1) {
                ns = 1;
            }
        }
        /*
         * Add to m_rrxn. m_rrxn is a vector of maps. m_rrxn has a length
         * equal to the total number of species for each species, there
         * exists a map, with the reaction number being the key, and the
         * reactant stoichiometric coefficient being the value.
         */
        m_rrxn[r.reactants[n]][rnum] = nsFlt;
        for (m = 0; m < ns; m++) {
            rk.push_back(r.reactants[n]);
        }
    }
    /*
     * Now that we have rk[], we add it into the vector<vector_int> m_reactants
     * in the rnum index spot. Thus m_reactants[rnum] yields a vector
     * of reactants for the rnum'th reaction
     */
    m_reactants.push_back(rk);
    std::vector<size_t> pk;
    size_t np = r.products.size();
    for (n = 0; n < np; n++) {
        nsFlt = r.pstoich[n];
        ns = (size_t) nsFlt;
        if ((doublereal) ns != nsFlt) {
            if (ns < 1) {
                ns = 1;
            }
        }
        /*
         * Add to m_prxn. m_prxn is a vector of maps. m_prxn has a length
         * equal to the total number of species for each species, there
         * exists a map, with the reaction number being the key, and the
         * product stoichiometric coefficient being the value.
         */
        m_prxn[r.products[n]][rnum] = nsFlt;
        for (m = 0; m < ns; m++) {
            pk.push_back(r.products[n]);
        }
    }
    /*
     * Now that we have pk[], we add it into the vector<vector_int> m_products
     * in the rnum index spot. Thus m_products[rnum] yields a vector
     * of products for the rnum'th reaction
     */
    m_products.push_back(pk);
    /*
     * Add this reaction to the stoichiometric coefficient manager. This
     * calculates rates of species production from reaction rates of
     * progress.
     */
    m_rxnstoich.add(reactionNumber(), r);
    /*
     * register reaction in lists of reversible and irreversible rxns.
     */
    if (r.reversible) {
        m_revindex.push_back(reactionNumber());
        m_nrev++;
    } else {
        m_irrev.push_back(reactionNumber());
        m_nirrev++;
    }
}
//===============================================================================================
void InterfaceKinetics::addPhase(thermo_t& thermo)
{
    Kinetics::addPhase(thermo);
    m_phaseExists.push_back(true);
    m_phaseIsStable.push_back(true);
}
//================================================================================================
/**
 * Prepare the class for the addition of reactions. This function
 * must be called after instantiation of the class, but before
 * any reactions are actually added to the mechanism.
 * This function calculates m_kk the number of species in all
 * phases participating in the reaction mechanism. We don't know
 * m_kk previously, before all phases have been added.
 */
void InterfaceKinetics::init()
{
    m_kk = 0;
    for (size_t n = 0; n < nPhases(); n++) {
        m_kk += thermo(n).nSpecies();
    }
    m_rrxn.resize(m_kk);
    m_prxn.resize(m_kk);
    m_conc.resize(m_kk);
    m_mu0.resize(m_kk);
    m_grt.resize(m_kk);
    m_pot.resize(m_kk, 0.0);
    m_phi.resize(nPhases(), 0.0);
}
//================================================================================================
/**
 * Finish adding reactions and prepare for use. This function
 * must be called after all reactions are entered into the mechanism
 * and before the mechanism is used to calculate reaction rates.
 *
 * Here, we resize work arrays based on the number of reactions,
 * since we don't know this number up to now.
 */
void InterfaceKinetics::finalize()
{
    Kinetics::finalize();
    size_t safe_reaction_size = std::max<size_t>(nReactions(), 1);
    m_rwork.resize(safe_reaction_size);
    size_t ks = reactionPhaseIndex();
    if (ks == npos) throw CanteraError("InterfaceKinetics::finalize",
                                           "no surface phase is present.");
    m_surf = (SurfPhase*)&thermo(ks);
    if (m_surf->nDim() != 2)
        throw CanteraError("InterfaceKinetics::finalize",
                           "expected interface dimension = 2, but got dimension = "
                           +int2str(m_surf->nDim()));

    m_StandardConc.resize(m_kk, 0.0);
    m_deltaG0.resize(safe_reaction_size, 0.0);
    m_ProdStanConcReac.resize(safe_reaction_size, 0.0);

    if (m_thermo.size() != m_phaseExists.size()) {
        throw CanteraError("InterfaceKinetics::finalize", "internal error");
    }

    // Guarantee that these arrays can be converted to double* even in the
    // special case where there are no reactions defined.
    if (!m_ii) {
        m_perturb.resize(1, 1.0);
    }

    m_finalized = true;
}

doublereal InterfaceKinetics::electrochem_beta(size_t irxn) const
{
    for (size_t i = 0; i < m_ctrxn.size(); i++) {
        if (m_ctrxn[i] == irxn) {
            return m_beta[i];
        }
    }
    return 0.0;
}

//================================================================================================
bool InterfaceKinetics::ready() const
{
    return (m_finalized);
}
//================================================================================================
// Advance the surface coverages in time
/*
 * @param tstep  Time value to advance the surface coverages
 */
void InterfaceKinetics::
advanceCoverages(doublereal tstep)
{
    if (m_integrator == 0) {
        vector<InterfaceKinetics*> k;
        k.push_back(this);
        m_integrator = new ImplicitSurfChem(k);
        m_integrator->initialize();
    }
    m_integrator->integrate(0.0, tstep);
    delete m_integrator;
    m_integrator = 0;
}
//================================================================================================
// Solve for the pseudo steady-state of the surface problem
/*
 * Solve for the steady state of the surface problem.
 * This is the same thing as the advanceCoverages() function,
 * but at infinite times.
 *
 * Note, a direct solve is carried out under the hood here,
 * to reduce the computational time.
 *
 * the integrator object is saved between calls to
 * reduce the computational cost of repeated calls.
 */
void InterfaceKinetics::
solvePseudoSteadyStateProblem(int ifuncOverride, doublereal timeScaleOverride)
{
    // create our own solver object
    if (m_integrator == 0) {
        vector<InterfaceKinetics*> k;
        k.push_back(this);
        m_integrator = new ImplicitSurfChem(k);
        m_integrator->initialize();
    }
    m_integrator->setIOFlag(m_ioFlag);
    /*
     * New direct method to go here
     */
    m_integrator->solvePseudoSteadyStateProblem(ifuncOverride, timeScaleOverride);
}
//================================================================================================

void InterfaceKinetics::setPhaseExistence(const size_t iphase, const bool exists)
{
    if (iphase >= m_thermo.size()) {
        throw CanteraError("InterfaceKinetics:setPhaseExistence", "out of bounds");
    }
    if (exists) {
        if (!m_phaseExists[iphase]) {
            m_phaseExistsCheck--;
            m_phaseExists[iphase] = true;
        }
        m_phaseIsStable[iphase] = true;
    } else {
        if (m_phaseExists[iphase]) {
            m_phaseExistsCheck++;
            m_phaseExists[iphase] = false;
        }
        m_phaseIsStable[iphase] = false;
    }
}
//================================================================================================
// Gets the phase existence int for the ith phase
/*
 * @param iphase  Phase Id
 *
 * @return Returns the int specifying whether the kinetics object thinks the phase exists
 *         or not. If it exists, then species in that phase can be a reactant in reactions.
 */
int InterfaceKinetics::phaseExistence(const int iphase) const
{
    if (iphase < 0 || iphase >= (int) m_thermo.size()) {
        throw CanteraError("InterfaceKinetics:phaseExistence()", "out of bounds");
    }
    return m_phaseExists[iphase];
}
//================================================================================================
// Gets the phase stability int for the ith phase
/*
 * @param iphase  Phase Id
 *
 * @return Returns the int specifying whether the kinetics object thinks the phase is stable
 *         with nonzero mole numbers.
 *         If it stable, then the kinetics object will allow for rates of production of
 *         of species in that phase that are positive.
 */
int InterfaceKinetics::phaseStability(const int iphase) const
{
    if (iphase < 0 || iphase >= (int) m_thermo.size()) {
        throw CanteraError("InterfaceKinetics:phaseStability()", "out of bounds");
    }
    return m_phaseIsStable[iphase];
}
//================================================================================================

void InterfaceKinetics::setPhaseStability(const int iphase, const int isStable)
{
    if (iphase < 0 || iphase >= (int) m_thermo.size()) {
        throw CanteraError("InterfaceKinetics:setPhaseStability", "out of bounds");
    }
    if (isStable) {
        m_phaseIsStable[iphase] = true;
    } else {
        m_phaseIsStable[iphase] = false;
    }
}

//================================================================================================
void EdgeKinetics::finalize()
{
    m_rwork.resize(std::max<size_t>(nReactions(), 1));
    size_t ks = reactionPhaseIndex();
    if (ks == npos) throw CanteraError("EdgeKinetics::finalize",
                                           "no edge phase is present.");
    m_surf = (SurfPhase*)&thermo(ks);
    if (m_surf->nDim() != 1)
        throw CanteraError("EdgeKinetics::finalize",
                           "expected interface dimension = 1, but got dimension = "
                           +int2str(m_surf->nDim()));

    // Guarantee that these arrays can be converted to double* even in the
    // special case where there are no reactions defined.
    if (!m_ii) {
        m_perturb.resize(1, 1.0);
    }

    m_finalized = true;
}
//================================================================================================
}


