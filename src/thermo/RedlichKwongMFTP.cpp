/**
 *  @file RedlichKwongMFTP.cpp
 * Definition file for a derived class of ThermoPhase that assumes either
 * an ideal gas or ideal solution approximation and handles
 * variable pressure standard state methods for calculating
 * thermodynamic properties (see \ref thermoprops and
 * class \link Cantera::RedlichKwongMFTP RedlichKwongMFTP\endlink).
 */
/*
 * Copyright (2005) Sandia Corporation. Under the terms of
 * Contract DE-AC04-94AL85000 with Sandia Corporation, the
 * U.S. Government retains certain rights in this software.
 */
/*
 *  $Author: hkmoffa $
 *  $Date: 2009-11-09 16:36:49 -0700 (Mon, 09 Nov 2009) $
 *  $Revision: 255 $
 */

// turn off warnings under Windows
#ifdef WIN32
#pragma warning(disable:4786)
#pragma warning(disable:4503)
#endif

#include "cantera/thermo/RedlichKwongMFTP.h"


#include "cantera/thermo/mix_defs.h"
#include "cantera/thermo/ThermoFactory.h"
#include "cantera/numerics/RootFind.h"

using namespace std;

namespace Cantera
{
#ifdef WITH_REAL_GASSES
//====================================================================================================================
/*
 * Default constructor
 */
RedlichKwongMFTP::RedlichKwongMFTP() :
    MixtureFugacityTP(),
    m_standardMixingRules(0),
    m_formTempParam(0),
    m_b_current(0.0),
    m_a_current(0.0),
    a_vec_Curr_(0),
    b_vec_Curr_(0),
    a_coeff_vec(0,0),
    m_pc_Species(0),
    m_tc_Species(0),
    m_vc_Species(0),
    NSolns_(0),
    m_pp(0),
    m_tmpV(0),
    m_partialMolarVolumes(0),
    dpdV_(0.0),
    dpdT_(0.0),
    dpdni_(0)
{
    Vroot_[0] = 0.0;
    Vroot_[1] = 0.0;
    Vroot_[2] = 0.0;
}
//====================================================================================================================
RedlichKwongMFTP::RedlichKwongMFTP(std::string infile, std::string id) :
    MixtureFugacityTP(),
    m_standardMixingRules(0),
    m_formTempParam(0),
    m_b_current(0.0),
    m_a_current(0.0),
    a_vec_Curr_(0),
    b_vec_Curr_(0),
    a_coeff_vec(0,0),
    m_pc_Species(0),
    m_tc_Species(0),
    m_vc_Species(0),
    NSolns_(0),
    m_pp(0),
    m_tmpV(0),
    m_partialMolarVolumes(0),
    dpdV_(0.0),
    dpdT_(0.0),
    dpdni_(0)
{
    Vroot_[0] = 0.0;
    Vroot_[1] = 0.0;
    Vroot_[2] = 0.0;
    XML_Node* root = get_XML_File(infile);
    if (id == "-") {
        id = "";
    }
    XML_Node* xphase = get_XML_NameID("phase", std::string("#")+id, root);
    if (!xphase) {
        throw CanteraError("newPhase",
                           "Couldn't find phase named \"" + id + "\" in file, " + infile);
    }
    importPhase(*xphase, this);
}
//====================================================================================================================
RedlichKwongMFTP::RedlichKwongMFTP(XML_Node& phaseRefRoot, std::string id) :
    MixtureFugacityTP(),
    m_standardMixingRules(0),
    m_formTempParam(0),
    m_b_current(0.0),
    m_a_current(0.0),
    a_vec_Curr_(0),
    b_vec_Curr_(0),
    a_coeff_vec(0,0),
    m_pc_Species(0),
    m_tc_Species(0),
    m_vc_Species(0),
    NSolns_(0),
    m_pp(0),
    m_tmpV(0),
    m_partialMolarVolumes(0),
    dpdV_(0.0),
    dpdT_(0.0),
    dpdni_(0)
{
    Vroot_[0] = 0.0;
    Vroot_[1] = 0.0;
    Vroot_[2] = 0.0;
    XML_Node* xphase = get_XML_NameID("phase", std::string("#")+id, &phaseRefRoot);
    if (!xphase) {
        throw CanteraError("RedlichKwongMFTP::RedlichKwongMFTP()","Couldn't find phase named \"" + id + "\" in XML node");
    }
    importPhase(*xphase, this);
}

//====================================================================================================================
RedlichKwongMFTP::RedlichKwongMFTP(int testProb) :
    MixtureFugacityTP(),
    m_standardMixingRules(0),
    m_formTempParam(0),
    m_b_current(0.0),
    m_a_current(0.0),
    a_vec_Curr_(0),
    b_vec_Curr_(0),
    a_coeff_vec(0,0),
    m_pc_Species(0),
    m_tc_Species(0),
    m_vc_Species(0),
    NSolns_(0),
    m_pp(0),
    m_tmpV(0),
    m_partialMolarVolumes(0),
    dpdV_(0.0),
    dpdT_(0.0),
    dpdni_(0)
{
    std::string infile = "co2_redlichkwong.xml";
    std::string id;
    if (testProb == 1) {
        infile = "co2_redlichkwong.xml";
        id = "carbondioxide";
    } else {
        throw CanteraError("", "test prob = 1 only");
    }
    XML_Node* root = get_XML_File(infile);
    if (id == "-") {
        id = "";
    }
    XML_Node* xphase = get_XML_NameID("phase", std::string("#")+id, root);
    if (!xphase) {
        throw CanteraError("newPhase", "Couldn't find phase named \"" + id + "\" in file, " + infile);
    }
    importPhase(*xphase, this);
}
//====================================================================================================================
/*
 * Copy Constructor:
 *
 *  Note this stuff will not work until the underlying phase
 *  has a working copy constructor.
 *
 *  The copy constructor just calls the assignment operator
 *  to do the heavy lifting.
 */
RedlichKwongMFTP::RedlichKwongMFTP(const RedlichKwongMFTP& b) :
    MixtureFugacityTP(),
    m_standardMixingRules(0),
    m_formTempParam(0),
    m_b_current(0.0),
    m_a_current(0.0),
    a_vec_Curr_(0),
    b_vec_Curr_(0),
    a_coeff_vec(0,0),
    m_pc_Species(0),
    m_tc_Species(0),
    m_vc_Species(0),
    NSolns_(0),
    m_pp(0),
    m_tmpV(0),
    m_partialMolarVolumes(0),
    dpdV_(0.0),
    dpdT_(0.0),
    dpdni_(0)
{
    *this = b;
}

//====================================================================================================================
/*
 * operator=()
 *
 *  Note this stuff will not work until the underlying phase
 *  has a working assignment operator
 */
RedlichKwongMFTP& RedlichKwongMFTP::
operator=(const RedlichKwongMFTP& b)
{
    if (&b != this) {
        /*
         * Mostly, this is a passthrough to the underlying
         * assignment operator for the ThermoPhae parent object.
         */
        MixtureFugacityTP::operator=(b);
        /*
         * However, we have to handle data that we own.
         */
        m_standardMixingRules = b.m_standardMixingRules;
        m_formTempParam = b.m_formTempParam;
        m_b_current = b.m_b_current;
        m_a_current = b.m_a_current;
        a_vec_Curr_ = b.a_vec_Curr_;
        b_vec_Curr_ = b.b_vec_Curr_;
        a_coeff_vec = b.a_coeff_vec;

        m_pc_Species = b.m_pc_Species;
        m_tc_Species = b.m_tc_Species;
        m_vc_Species = b.m_vc_Species;
        NSolns_ = b.NSolns_;
        Vroot_[0] = b.Vroot_[0];
        Vroot_[1] = b.Vroot_[1];
        Vroot_[2] = b.Vroot_[2];
        m_pp       = b.m_pp;
        m_tmpV     = b.m_tmpV;
        m_partialMolarVolumes = b.m_partialMolarVolumes;
        dpdV_ = b.dpdV_;
        dpdT_ = b.dpdT_;
        dpdni_ = b.dpdni_;
    }
    return *this;
}
//====================================================================================================================
/*
 * ~RedlichKwongMFTP():   (virtual)
 *
 */
RedlichKwongMFTP::~RedlichKwongMFTP()
{
}
//====================================================================================================================
/*
 * Duplication function.
 *  This calls the copy constructor for this object.
 */
ThermoPhase* RedlichKwongMFTP::duplMyselfAsThermoPhase() const
{
    RedlichKwongMFTP* vptp = new RedlichKwongMFTP(*this);
    return (ThermoPhase*) vptp;
}
//====================================================================================================================
int RedlichKwongMFTP::eosType() const
{
    return cRedlichKwongMFTP;
}

//====================================================================================================================
/*
 * ------------Molar Thermodynamic Properties -------------------------
 */
//====================================================================================================================
// Molar enthalpy. Units: J/kmol.
doublereal RedlichKwongMFTP::enthalpy_mole() const
{
    _updateReferenceStateThermo();
    doublereal rt = _RT();
    doublereal h_ideal = rt * mean_X(DATA_PTR(m_h0_RT));
    doublereal h_nonideal = hresid();
    return (h_ideal + h_nonideal);
}
//====================================================================================================================
// Molar internal energy. Units: J/kmol.
doublereal RedlichKwongMFTP::intEnergy_mole() const
{
    doublereal p0 = pressure();
    doublereal md = molarDensity();
    return (enthalpy_mole() - p0 / md);
}
//====================================================================================================================
// Molar entropy. Units: J/kmol/K.
doublereal RedlichKwongMFTP::entropy_mole() const
{
    _updateReferenceStateThermo();
    doublereal sr_ideal =  GasConstant * (mean_X(DATA_PTR(m_s0_R))
                                          - sum_xlogx() - std::log(pressure()/m_spthermo->refPressure()));
    doublereal sr_nonideal = sresid();
    return (sr_ideal + sr_nonideal);
}
//====================================================================================================================
// Molar Gibbs function. Units: J/kmol.
doublereal RedlichKwongMFTP::gibbs_mole() const
{
    return enthalpy_mole() - temperature() * entropy_mole();
}
//====================================================================================================================
/// Molar heat capacity at constant pressure. Units: J/kmol/K.
doublereal RedlichKwongMFTP::cp_mole() const
{
    _updateReferenceStateThermo();
    doublereal TKelvin = temperature();
    doublereal sqt = sqrt(TKelvin);
    doublereal mv = molarVolume();
    doublereal vpb = mv + m_b_current;
    pressureDerivatives();
    doublereal cpref = GasConstant * mean_X(DATA_PTR(m_cp0_R));
    doublereal dadt = da_dt();
    doublereal fac = TKelvin * dadt - 3.0 * m_a_current / 2.0;
    doublereal dHdT_V = (cpref + mv * dpdT_ - GasConstant - 1.0 / (2.0 * m_b_current * TKelvin * sqt) * log(vpb/mv) * fac
                         +1.0/(m_b_current * sqt) * log(vpb/mv) * (-0.5 * dadt));
    double cp = dHdT_V - (mv + TKelvin * dpdT_ / dpdV_) * dpdT_;
    return cp;
}
//====================================================================================================================
/// Molar heat capacity at constant volume. Units: J/kmol/K.
doublereal RedlichKwongMFTP::cv_mole() const
{
    throw CanteraError("", "unimplemented");
    return cp_mole() - GasConstant;
}
//====================================================================================================================
// Return the thermodynamic pressure (Pa).
/*
 *  Since the mass density, temperature, and mass fractions are stored,
 *  this method uses these values to implement the
 *  mechanical equation of state \f$ P(T, \rho, Y_1, \dots,  Y_K) \f$.
 *
 * \f[
 *    P = \frac{RT}{v-b_{mix}} - \frac{a_{mix}}{T^{0.5} v \left( v + b_{mix} \right) }
 * \f]
 *
 */
doublereal RedlichKwongMFTP::pressure() const
{


#ifdef DEBUG_MODE
    _updateReferenceStateThermo();

    //  Get a copy of the private variables stored in the State object
    double rho = density();
    doublereal T = temperature();
    doublereal mmw = meanMolecularWeight();
    double molarV = mmw / rho;

    double pp = GasConstant * T/(molarV - m_b_current) - m_a_current/(sqrt(T) * molarV * (molarV + m_b_current));

    if (fabs(pp -m_Pcurrent) > 1.0E-5 * fabs(m_Pcurrent)) {
        throw CanteraError(" RedlichKwongMFTP::pressure()", "setState broken down, maybe");
    }
#endif

    return m_Pcurrent;
}
//====================================================================================================================
void RedlichKwongMFTP::calcDensity()
{
    /*
     * Calculate the molarVolume of the solution (m**3 kmol-1)
     */

    const doublereal* const dtmp = moleFractdivMMW();
    getPartialMolarVolumes(DATA_PTR(m_tmpV));
    double invDens = dot(m_tmpV.begin(), m_tmpV.end(), dtmp);
    /*
     * Set the density in the parent State object directly,
     * by calling the State::setDensity() function.
     */
    double dens = 1.0/invDens;
    State::setDensity(dens);

}

//====================================================================================================================
void RedlichKwongMFTP::setTemperature(const doublereal temp)
{
    State::setTemperature(temp);
    _updateReferenceStateThermo();
    updateAB();
}
//====================================================================================================================
void RedlichKwongMFTP::setMassFractions(const doublereal* const x)
{
    MixtureFugacityTP::setMassFractions(x);
    updateAB();
}
//====================================================================================================================
void RedlichKwongMFTP::setMassFractions_NoNorm(const doublereal* const x)
{
    MixtureFugacityTP::setMassFractions_NoNorm(x);
    updateAB();
}
//====================================================================================================================
void RedlichKwongMFTP::setMoleFractions(const doublereal* const x)
{
    MixtureFugacityTP::setMoleFractions(x);
    updateAB();
}
//====================================================================================================================
void RedlichKwongMFTP::setMoleFractions_NoNorm(const doublereal* const x)
{
    MixtureFugacityTP::setMoleFractions(x);
    updateAB();
}
//====================================================================================================================
void RedlichKwongMFTP::setConcentrations(const doublereal* const c)
{
    MixtureFugacityTP::setConcentrations(c);
    updateAB();
}

//====================================================================================================================
doublereal RedlichKwongMFTP::isothermalCompressibility() const
{


    throw CanteraError("RedlichKwongMFTP::isothermalCompressibility() ",
                       "not implemented");

    return 0.0;
}
//====================================================================================================================
void RedlichKwongMFTP::getActivityConcentrations(doublereal* c) const
{

    int k;
    getPartialMolarVolumes(DATA_PTR(m_partialMolarVolumes));

    for (k = 0; k < m_kk; k++) {
        c[k] = moleFraction(k) / m_partialMolarVolumes[k];
    }
}
//====================================================================================================================
/*
 * Returns the standard concentration \f$ C^0_k \f$, which is used to normalize
 * the generalized concentration.
 */
doublereal RedlichKwongMFTP::standardConcentration(int k) const
{

    getStandardVolumes(DATA_PTR(m_tmpV));

    return 1.0 / m_tmpV[k];



}
//====================================================================================================================
/*
 * Returns the natural logarithm of the standard
 * concentration of the kth species
 */
doublereal RedlichKwongMFTP::logStandardConc(int k) const
{
    double c = standardConcentration(k);
    double lc = std::log(c);
    return lc;
}
//====================================================================================================================
/*
 *
 * getUnitsStandardConcentration()
 *
 * Returns the units of the standard and general concentrations
 * Note they have the same units, as their divisor is
 * defined to be equal to the activity of the kth species
 * in the solution, which is unitless.
 *
 * This routine is used in print out applications where the
 * units are needed. Usually, MKS units are assumed throughout
 * the program and in the XML input files.
 *
 *  uA[0] = kmol units - default  = 1
 *  uA[1] = m    units - default  = -nDim(), the number of spatial
 *                                dimensions in the Phase class.
 *  uA[2] = kg   units - default  = 0;
 *  uA[3] = Pa(pressure) units - default = 0;
 *  uA[4] = Temperature units - default = 0;
 *  uA[5] = time units - default = 0
 *
 *  For EOS types other than cIdealSolidSolnPhase1, the default
 *  kmol/m3 holds for standard concentration units. For
 *  cIdealSolidSolnPhase0 type, the standard concentrtion is
 *  unitless.
 */
void RedlichKwongMFTP::getUnitsStandardConc(double* uA, int, int sizeUA) const
{
    //int eos = eosType();

    for (int i = 0; i < sizeUA; i++) {
        if (i == 0) {
            uA[0] = 1.0;
        }
        if (i == 1) {
            uA[1] = -nDim();
        }
        if (i == 2) {
            uA[2] = 0.0;
        }
        if (i == 3) {
            uA[3] = 0.0;
        }
        if (i == 4) {
            uA[4] = 0.0;
        }
        if (i == 5) {
            uA[5] = 0.0;
        }
    }

}

//====================================================================================================================
//! Get the array of non-dimensional activity coefficients at
//! the current solution temperature, pressure, and solution concentration.
/*!
 *  For ideal gases, the activity coefficients are all equal to one.
 *
 * @param ac Output vector of activity coefficients. Length: m_kk.
 */
void RedlichKwongMFTP::getActivityCoefficients(doublereal* ac) const
{
    doublereal TKelvin = temperature();
    doublereal rt = TKelvin * GasConstant;
    doublereal mv = molarVolume();
    doublereal sqt = sqrt(TKelvin);
    doublereal vpb = mv + m_b_current;
    doublereal vmb = mv - m_b_current;

    for (int k = 0; k < m_kk; k++) {
        m_pp[k] = 0.0;
        for (int i = 0; i < m_kk; i++) {
            int counter = k + m_kk*i;
            m_pp[k] += moleFractions_[i] * a_vec_Curr_[counter];
        }
    }
    doublereal pres = pressure();

    for (int k = 0; k < m_kk; k++) {
        ac[k] = (- rt * log(pres * mv / rt)
                 + rt * log(mv / vmb)
                 + rt * b_vec_Curr_[k] / vmb
                 - 2.0 * m_pp[k] / (m_b_current * sqt) * log(vpb/mv)
                 + m_a_current *  b_vec_Curr_[k] / (m_b_current * m_b_current * sqt) * log(vpb/mv)
                 - m_a_current / (m_b_current * sqt) * (b_vec_Curr_[k]/vpb)
                );
    }
    for (int k = 0; k < m_kk; k++) {
        ac[k] = exp(ac[k]/rt);
    }
}
//====================================================================================================================
/*
 * ---- Partial Molar Properties of the Solution -----------------
 */
//====================================================================================================================
/*
 * Get the array of non-dimensional species chemical potentials
 * These are partial molar Gibbs free energies.
 * \f$ \mu_k / \hat R T \f$.
 * Units: unitless
 *
 * We close the loop on this function, here, calling
 * getChemPotentials() and then dividing by RT.
 */
void RedlichKwongMFTP::getChemPotentials_RT(doublereal* muRT) const
{
    getChemPotentials(muRT);
    doublereal invRT = 1.0 / _RT();
    for (int k = 0; k < m_kk; k++) {
        muRT[k] *= invRT;
    }
}
//====================================================================================================================
void RedlichKwongMFTP::getChemPotentials(doublereal* mu) const
{
    getGibbs_ref(mu);
    doublereal xx;
    doublereal rt = temperature() * GasConstant;
    for (int k = 0; k < m_kk; k++) {
        xx = fmaxx(SmallNumber, moleFraction(k));
        mu[k] += rt*(log(xx));
    }

    doublereal TKelvin = temperature();
    doublereal mv = molarVolume();
    doublereal sqt = sqrt(TKelvin);
    doublereal vpb = mv + m_b_current;
    doublereal vmb = mv - m_b_current;

    for (int k = 0; k < m_kk; k++) {
        m_pp[k] = 0.0;
        for (int i = 0; i < m_kk; i++) {
            int counter = k + m_kk*i;
            m_pp[k] += moleFractions_[i] * a_vec_Curr_[counter];
        }
    }
    doublereal pres = pressure();
    doublereal refP = refPressure();

    for (int k = 0; k < m_kk; k++) {
        mu[k] += (rt * log(pres/refP) - rt * log(pres * mv / rt)
                  + rt * log(mv / vmb)
                  + rt * b_vec_Curr_[k] / vmb
                  - 2.0 * m_pp[k] / (m_b_current * sqt) * log(vpb/mv)
                  + m_a_current *  b_vec_Curr_[k] / (m_b_current * m_b_current * sqt) * log(vpb/mv)
                  - m_a_current / (m_b_current * sqt) * (b_vec_Curr_[k]/vpb)
                 );
    }
}
//====================================================================================================================
void RedlichKwongMFTP::getPartialMolarEnthalpies(doublereal* hbar) const
{
    /*
     *  First we get the reference state contributions
     */
    getEnthalpy_RT_ref(hbar);
    doublereal rt = GasConstant * temperature();
    scale(hbar, hbar+m_kk, hbar, rt);

    /*
     * We calculate dpdni_
     */
    doublereal TKelvin = temperature();
    doublereal mv = molarVolume();
    doublereal sqt = sqrt(TKelvin);

    doublereal vpb = mv + m_b_current;
    doublereal vmb = mv - m_b_current;

    for (int k = 0; k < m_kk; k++) {
        m_pp[k] = 0.0;
        for (int i = 0; i < m_kk; i++) {
            int counter = k + m_kk*i;
            m_pp[k] += moleFractions_[i] * a_vec_Curr_[counter];
        }
    }



    for (int k = 0; k < m_kk; k++) {
        dpdni_[k] = rt/vmb + rt * b_vec_Curr_[k] / (vmb * vmb) - 2.0 * m_pp[k] / (sqt * mv * vpb)
                    + m_a_current * b_vec_Curr_[k]/(sqt * mv * vpb * vpb);
    }
    doublereal dadt = da_dt();
    doublereal fac = TKelvin * dadt - 3.0 * m_a_current / 2.0;

    for (int k = 0; k < m_kk; k++) {
        m_tmpV[k] = 0.0;
        for (int i = 0; i < m_kk; i++) {
            int counter = k + m_kk*i;
            m_tmpV[k] += 2.0 * moleFractions_[i] * TKelvin * a_coeff_vec(1,counter) - 3.0 *  moleFractions_[i] * a_vec_Curr_[counter];
        }
    }

    pressureDerivatives();
    doublereal fac2 = mv + TKelvin * dpdT_ / dpdV_;

    for (int k = 0; k < m_kk; k++) {
        double hE_v = (mv * dpdni_[k] - rt -  b_vec_Curr_[k]/ (m_b_current * m_b_current * sqt) * log(vpb/mv)*fac
                       + 1.0 / (m_b_current * sqt) * log(vpb/mv) * m_tmpV[k]
                       +  b_vec_Curr_[k] / vpb / (m_b_current * sqt) * fac);
        hbar[k] = hbar[k] + hE_v;


        hbar[k] -= fac2 * dpdni_[k];
    }

}
//====================================================================================================================
void RedlichKwongMFTP::getPartialMolarEntropies(doublereal* sbar) const
{
    getEntropy_R_ref(sbar);
    doublereal r = GasConstant;
    scale(sbar, sbar+m_kk, sbar, r);
    doublereal TKelvin = temperature();
    doublereal sqt = sqrt(TKelvin);
    doublereal mv = molarVolume();
    doublereal refP = refPressure();

    for (int k = 0; k < m_kk; k++) {
        doublereal xx = fmaxx(SmallNumber, moleFraction(k));
        sbar[k] += r * (- log(xx));
    }

    for (int k = 0; k < m_kk; k++) {
        m_pp[k] = 0.0;
        for (int i = 0; i < m_kk; i++) {
            int counter = k + m_kk*i;
            m_pp[k] += moleFractions_[i] * a_vec_Curr_[counter];
        }
    }

    for (int k = 0; k < m_kk; k++) {
        m_tmpV[k] = 0.0;
        for (int i = 0; i < m_kk; i++) {
            int counter = k + m_kk*i;
            m_tmpV[k] += moleFractions_[i] * a_coeff_vec(1,counter);
        }
    }


    doublereal dadt = da_dt();
    doublereal fac = dadt -  m_a_current / (2.0 * TKelvin);
    doublereal vmb = mv - m_b_current;
    doublereal vpb = mv + m_b_current;


    for (int k = 0; k < m_kk; k++) {
        sbar[k] -=(GasConstant * log(GasConstant * TKelvin / (refP * mv))
                   + GasConstant
                   + GasConstant * log(mv/vmb)
                   + GasConstant * b_vec_Curr_[k]/vmb
                   + m_pp[k]/(m_b_current * TKelvin * sqt) * log(vpb/mv)
                   - 2.0 * m_tmpV[k]/(m_b_current * sqt) * log(vpb/mv)
                   + b_vec_Curr_[k] / (m_b_current * m_b_current * sqt) * log(vpb/mv) * fac
                   - 1.0 / (m_b_current * sqt) *  b_vec_Curr_[k] / vpb * fac
                  ) ;
    }

    pressureDerivatives();
    getPartialMolarVolumes(DATA_PTR(m_partialMolarVolumes));
    for (int k = 0; k < m_kk; k++) {
        sbar[k] -= -m_partialMolarVolumes[k] * dpdT_;
    }
}
//====================================================================================================================
void RedlichKwongMFTP::getPartialMolarIntEnergies(doublereal* ubar) const
{
    getIntEnergy_RT(ubar);
    doublereal rt = GasConstant * temperature();
    scale(ubar, ubar+m_kk, ubar, rt);
}
//====================================================================================================================
void RedlichKwongMFTP::getPartialMolarCp(doublereal* cpbar) const
{
    getCp_R(cpbar);
    doublereal r = GasConstant;
    scale(cpbar, cpbar+m_kk, cpbar, r);
}
//====================================================================================================================
void RedlichKwongMFTP::getPartialMolarVolumes(doublereal* vbar) const
{
    // getStandardVolumes(vbar);


    for (int k = 0; k < m_kk; k++) {
        m_pp[k] = 0.0;
        for (int i = 0; i < m_kk; i++) {
            int counter = k + m_kk*i;
            m_pp[k] += moleFractions_[i] * a_vec_Curr_[counter];
        }
    }

    for (int k = 0; k < m_kk; k++) {
        m_tmpV[k] = 0.0;
        for (int i = 0; i < m_kk; i++) {
            int counter = k + m_kk*i;
            m_tmpV[k] += moleFractions_[i] * a_coeff_vec(1,counter);
        }
    }

    doublereal TKelvin = temperature();
    doublereal sqt = sqrt(TKelvin);
    doublereal mv = molarVolume();

    doublereal rt = GasConstant * TKelvin;

    doublereal vmb = mv - m_b_current;
    doublereal vpb = mv + m_b_current;

    for (int k = 0; k < m_kk; k++) {

        doublereal num = (rt + rt * m_b_current/ vmb + rt * b_vec_Curr_[k] / vmb
                          + rt *  m_b_current * b_vec_Curr_[k] /(vmb * vmb)
                          - 2.0 * m_pp[k] / (sqt * vpb)
                          + m_a_current *  b_vec_Curr_[k] / (sqt * vpb * vpb)
                         );

        doublereal denom = (m_Pcurrent + rt * m_b_current/(vmb * vmb) - m_a_current / (sqt * vpb * vpb)
                           );

        vbar[k] = num / denom;
    }

}
//====================================================================================================================
doublereal RedlichKwongMFTP::critTemperature() const
{
    double pc, tc, vc;
    double a0 = 0.0;
    double aT = 0.0;
    for (int i = 0; i < m_kk; i++) {
        for (int j = 0; j <m_kk; j++) {
            int counter = i + m_kk * j;
            a0 += moleFractions_[i] * moleFractions_[j] * a_coeff_vec(0, counter);
            aT += moleFractions_[i] * moleFractions_[j] * a_coeff_vec(1, counter);
        }
    }
    calcCriticalConditions(m_a_current, m_b_current, a0, aT, pc, tc, vc);
    return tc;
}
//====================================================================================================================
doublereal RedlichKwongMFTP::critPressure() const
{
    double pc, tc, vc;
    double a0 = 0.0;
    double aT = 0.0;
    for (int i = 0; i < m_kk; i++) {
        for (int j = 0; j <m_kk; j++) {
            int counter = i + m_kk * j;
            a0 += moleFractions_[i] * moleFractions_[j] * a_coeff_vec(0, counter);
            aT += moleFractions_[i] * moleFractions_[j] * a_coeff_vec(1, counter);
        }
    }
    calcCriticalConditions(m_a_current, m_b_current, a0, aT, pc, tc, vc);

    return pc;
}
//====================================================================================================================
doublereal RedlichKwongMFTP::critDensity() const
{
    double pc, tc, vc;
    double a0 = 0.0;
    double aT = 0.0;
    for (int i = 0; i < m_kk; i++) {
        for (int j = 0; j <m_kk; j++) {
            int counter = i + m_kk * j;
            a0 += moleFractions_[i] * moleFractions_[j] *a_coeff_vec(0, counter);
            aT += moleFractions_[i] * moleFractions_[j] *a_coeff_vec(1, counter);
        }
    }
    calcCriticalConditions(m_a_current, m_b_current, a0, aT, pc, tc, vc);

    double mmw = meanMolecularWeight();
    return mmw / vc;
}
//====================================================================================================================

/*
 * ----- Thermodynamic Values for the Species Reference States ----
 */


//====================================================================================================================

/*
 * Perform initializations after all species have been
 * added.
 */
void RedlichKwongMFTP::initThermo()
{
    initLengths();
    MixtureFugacityTP::initThermo();
}

//====================================================================================================================
void RedlichKwongMFTP::setToEquilState(const doublereal* mu_RT)
{
    double tmp, tmp2;
    _updateReferenceStateThermo();

    getGibbs_RT_ref(DATA_PTR(m_tmpV));


    /*
     * Within the method, we protect against inf results if the
     * exponent is too high.
     *
     * If it is too low, we set
     * the partial pressure to zero. This capability is needed
     * by the elemental potential method.
     */
    doublereal pres = 0.0;
    double m_p0 = refPressure();
    for (int k = 0; k < m_kk; k++) {
        tmp = -m_tmpV[k] + mu_RT[k];
        if (tmp < -600.) {
            m_pp[k] = 0.0;
        } else if (tmp > 500.0) {
            tmp2 = tmp / 500.;
            tmp2 *= tmp2;
            m_pp[k] = m_p0 * exp(500.) * tmp2;
        } else {
            m_pp[k] = m_p0 * exp(tmp);
        }
        pres += m_pp[k];
    }
    // set state
    setState_PX(pres, &m_pp[0]);
}
//====================================================================================================================
/*
 * Initialize the internal lengths.
 *       (this is not a virtual function)
 */
void RedlichKwongMFTP::initLengths()
{


    a_vec_Curr_.resize(m_kk * m_kk, 0.0);
    b_vec_Curr_.resize(m_kk, 0.0);

    a_coeff_vec.resize(2, m_kk * m_kk, 0.0);


    m_pc_Species.resize(m_kk, 0.0);
    m_tc_Species.resize(m_kk, 0.0);
    m_vc_Species.resize(m_kk, 0.0);


    m_pp.resize(m_kk, 0.0);
    m_tmpV.resize(m_kk, 0.0);
    m_partialMolarVolumes.resize(m_kk, 0.0);
    dpdni_.resize(m_kk, 0.0);
}
//====================================================================================================================
/*
 *   Import and initialize a ThermoPhase object
 *
 * param phaseNode This object must be the phase node of a
 *             complete XML tree
 *             description of the phase, including all of the
 *             species data. In other words while "phase" must
 *             point to an XML phase object, it must have
 *             sibling nodes "speciesData" that describe
 *             the species in the phase.
 * param id   ID of the phase. If nonnull, a check is done
 *             to see if phaseNode is pointing to the phase
 *             with the correct id.
 *
 * This routine initializes the lengths in the current object and
 * then calls the parent routine.
 */
void RedlichKwongMFTP::initThermoXML(XML_Node& phaseNode, std::string id)
{
    RedlichKwongMFTP::initLengths();

    /*
     *  Check the model parameter for the Redlich-Kwong equation of state
     *  two are allowed
     *        RedlichKwong        mixture of species, each of which are RK fluids
     *        RedlichKwongMFTP    mixture of species with cross term coefficients
     */
    if (phaseNode.hasChild("thermo")) {
        XML_Node& thermoNode = phaseNode.child("thermo");
        std::string model = thermoNode["model"];
        if (model == "RedlichKwong") {
            m_standardMixingRules = 1;
        } else if (model == "RedlichKwongMFTP") {
            m_standardMixingRules = 0;
        } else {
            throw CanteraError("RedlichKwongMFTP::initThermoXML",
                               "Unknown thermo model : " + model);
        }


        /*
         * Go get all of the coefficients and factors in the
         * activityCoefficients XML block
         */
        XML_Node* acNodePtr = 0;
        if (thermoNode.hasChild("activityCoefficients")) {
            XML_Node& acNode = thermoNode.child("activityCoefficients");
            acNodePtr = &acNode;
            int nC = acNode.nChildren();

            /*
             * Loop through the children getting multiple instances of
             * parameters
             */
            for (int i = 0; i < nC; i++) {
                XML_Node& xmlACChild = acNodePtr->child(i);
                string stemp = xmlACChild.name();
                string nodeName = lowercase(stemp);
                /*
                 * Process a binary salt field, or any of the other XML fields
                 * that make up the Pitzer Database. Entries will be ignored
                 * if any of the species in the entry isn't in the solution.
                 */
                if (nodeName == "purefluidparameters") {
                    readXMLPureFluid(xmlACChild);
                }
            }
            if (m_standardMixingRules == 1) {
                applyStandardMixingRules();
            }
            /*
             * Loop through the children getting multiple instances of
             * parameters
             */
            for (int i = 0; i < nC; i++) {
                XML_Node& xmlACChild = acNodePtr->child(i);
                string stemp = xmlACChild.name();
                string nodeName = lowercase(stemp);
                /*
                 * Process a binary salt field, or any of the other XML fields
                 * that make up the Pitzer Database. Entries will be ignored
                 * if any of the species in the entry isn't in the solution.
                 */
                if (nodeName == "crossfluidparameters") {
                    readXMLCrossFluid(xmlACChild);
                }
            }

        }
    }

    for (int i = 0; i < m_kk; i++) {
        double a0coeff =  a_coeff_vec(0, i*m_kk + i);
        double aTcoeff =  a_coeff_vec(1, i*m_kk + i);
        double ai =  a0coeff + aTcoeff * 500.;
        double bi = b_vec_Curr_[i];
        calcCriticalConditions(ai, bi, a0coeff, aTcoeff, m_pc_Species[i], m_tc_Species[i], m_vc_Species[i]);
    }

    MixtureFugacityTP::initThermoXML(phaseNode, id);
}
//====================================================================================================================

void RedlichKwongMFTP::readXMLPureFluid(XML_Node& PureFluidParam)
{
    vector_fp vParams;
    string xname = PureFluidParam.name();
    if (xname != "pureFluidParameters") {
        throw CanteraError("RedlichKwongMFTP::readXMLPureFluid",
                           "Incorrect name for processing this routine: " + xname);
    }

    /*
     *  Read the species
     *  Find the index of the species in the current phase. It's not an error to not find the species
     */
    string iName = PureFluidParam.attrib("species");
    if (iName == "") {
        throw CanteraError("RedlichKwongMFTP::readXMLPureFluid", "no species attribute");
    }
    int iSpecies = speciesIndex(iName);
    if (iSpecies < 0) {
        return;
    }
    int counter = iSpecies + m_kk * iSpecies;
    int nParamsExpected, nParamsFound;
    int num = PureFluidParam.nChildren();
    for (int iChild = 0; iChild < num; iChild++) {
        XML_Node& xmlChild = PureFluidParam.child(iChild);
        string stemp = xmlChild.name();
        string nodeName = lowercase(stemp);

        if (nodeName == "a_coeff") {
            string iModel = lowercase(xmlChild.attrib("model"));
            if (iModel == "constant") {
                nParamsExpected = 1;
            } else if (iModel == "linear_a") {
                nParamsExpected = 2;
                if (m_formTempParam == 0) {
                    m_formTempParam = 1;
                }
            } else {
                throw CanteraError("", "unknown model");
            }

            ctml::getFloatArray(xmlChild, vParams, true, "Pascal-m6/kmol2", "a_coeff");
            nParamsFound = vParams.size();
            if (nParamsFound != nParamsExpected) {
                throw CanteraError("RedlichKwongMFTP::readXMLPureFluid(for a_coeff" + iName + ")",
                                   "wrong number of params found");
            }

            for (int i = 0; i < nParamsFound; i++) {
                a_coeff_vec(i, counter) = vParams[i];
            }
        } else if (nodeName == "b_coeff") {
            ctml::getFloatArray(xmlChild, vParams, true, "m3/kmol", "b_coeff");
            nParamsFound = vParams.size();
            if (nParamsFound != 1) {
                throw CanteraError("RedlichKwongMFTP::readXMLPureFluid(for b_coeff" + iName + ")",
                                   "wrong number of params found");
            }
            b_vec_Curr_[iSpecies] = vParams[0];
        }
    }
}
//====================================================================================================================
void RedlichKwongMFTP::applyStandardMixingRules()
{
    int nParam = 2;
    for (int i = 0; i < m_kk; i++) {
        int icounter = i + m_kk * i;
        for (int j = 0; j < m_kk; j++) {
            if (i != j) {
                int counter = i + m_kk * j;
                int jcounter = j + m_kk * j;
                for (int n = 0; n < nParam; n++) {
                    a_coeff_vec(n, counter) = sqrt(a_coeff_vec(n, icounter) * a_coeff_vec(n, jcounter));
                }
            }
        }
    }
}
//====================================================================================================================

void RedlichKwongMFTP::readXMLCrossFluid(XML_Node& CrossFluidParam)
{
    vector_fp vParams;
    string xname = CrossFluidParam.name();
    if (xname != "crossFluidParameters") {
        throw CanteraError("RedlichKwongMFTP::readXMLCrossFluid",
                           "Incorrect name for processing this routine: " + xname);
    }

    /*
     *  Read the species
     *  Find the index of the species in the current phase. It's not an error to not find the species
     */
    string iName = CrossFluidParam.attrib("species1");
    if (iName == "") {
        throw CanteraError("RedlichKwongMFTP::readXMLCrossFluid", "no species1 attribute");
    }
    int iSpecies = speciesIndex(iName);
    if (iSpecies < 0) {
        return;
    }
    string jName = CrossFluidParam.attrib("species2");
    if (iName == "") {
        throw CanteraError("RedlichKwongMFTP::readXMLCrossFluid", "no species2 attribute");
    }
    int jSpecies = speciesIndex(jName);
    if (jSpecies < 0) {
        return;
    }

    int counter = iSpecies + m_kk * jSpecies;
    int counter0 = jSpecies + m_kk * iSpecies;
    int nParamsExpected, nParamsFound;
    int num = CrossFluidParam.nChildren();
    for (int iChild = 0; iChild < num; iChild++) {
        XML_Node& xmlChild = CrossFluidParam.child(iChild);
        string stemp = xmlChild.name();
        string nodeName = lowercase(stemp);

        if (nodeName == "a_coeff") {
            string iModel = lowercase(xmlChild.attrib("model"));
            if (iModel == "constant") {
                nParamsExpected = 1;
            } else if (iModel == "linear_a") {
                nParamsExpected = 2;
                if (m_formTempParam == 0) {
                    m_formTempParam = 1;
                }
            } else {
                throw CanteraError("", "unknown model");
            }

            ctml::getFloatArray(xmlChild, vParams, true, "Pascal-m6/kmol2", "a_coeff");
            nParamsFound = vParams.size();
            if (nParamsFound != nParamsExpected) {
                throw CanteraError("RedlichKwongMFTP::readXMLCrossFluid(for a_coeff" + iName + ")",
                                   "wrong number of params found");
            }

            for (int i = 0; i < nParamsFound; i++) {
                a_coeff_vec(i, counter) = vParams[i];
                a_coeff_vec(i, counter0) = vParams[i];
            }
        }
    }
}
//====================================================================================================================
void RedlichKwongMFTP::setParametersFromXML(const XML_Node& thermoNode)
{
    MixtureFugacityTP::setParametersFromXML(thermoNode);
    std::string model = thermoNode["model"];


}
//====================================================================================================================
// Calculate the deviation terms for the total entropy of the mixture from the
// ideal gas mixture
/*
 *  Here we use the current state conditions
 *
 * @return  Returns the change in entropy in units of J kmol-1 K-1.
 */
doublereal RedlichKwongMFTP::sresid() const
{
    // note this agrees with tpx
    doublereal rho = density();
    doublereal mmw = meanMolecularWeight();
    doublereal molarV = mmw / rho;
    double hh = m_b_current / molarV;
    doublereal zz = z();
    doublereal dadt = da_dt();
    doublereal T = temperature();
    doublereal sqT = sqrt(T);
    doublereal fac = dadt - m_a_current / (2.0 * T);
    double sresid_mol_R =  log(zz*(1.0 - hh)) + log(1.0 + hh) * fac / (sqT * GasConstant * m_b_current);
    double sp = GasConstant * sresid_mol_R;
    return sp;
}
//====================================================================================================================
// Calculate the deviation terms for the total enthalpy of the mixture from the
// ideal gas mixture
/*
 *  Here we use the current state conditions
 *
 * @return  Returns the change in entropy in units of J kmol-1.
 */
doublereal RedlichKwongMFTP::hresid() const
{
    // note this agrees with tpx
    doublereal rho = density();
    doublereal mmw = meanMolecularWeight();
    doublereal molarV = mmw / rho;
    double hh = m_b_current / molarV;
    doublereal zz = z();
    doublereal dadt = da_dt();
    doublereal T = temperature();
    doublereal sqT = sqrt(T);
    doublereal fac = T * dadt - 3.0 *m_a_current / (2.0);
    double hresid_mol = GasConstant * T * (zz - 1.0) + fac * log(1.0 + hh) / (sqT * m_b_current);
    return hresid_mol;
}
//====================================================================================================================
// Estimate for the molar volume of the liquid
/*
 *   Note: this is only used as a starting guess for later routines that actually calculate an
 *  accurate value for the liquid molar volume.
 *  This routine doesn't change the state of the system.
 *
 *  @param TKelvin  temperature in kelvin
 *  @param pres     Pressure in Pa. This is used as an initial guess. If the routine
 *                  needs to change the pressure to find a stable liquid state, the
 *                  new pressure is returned in this variable.
 *
 *  @return Returns the estimate of the liquid volume.  If the liquid can't be found, this
 *          routine returns -1.
 */
doublereal RedlichKwongMFTP::liquidVolEst(doublereal TKelvin, doublereal& presGuess) const
{

    double v = m_b_current * 1.1;
    double atmp;
    double btmp;
    calculateAB(TKelvin, atmp, btmp);

    doublereal pres = presGuess;
    double pp = psatEst(TKelvin);
    if (pres < pp) {
        pres = pp;
    }
    double Vroot[3];

#ifdef NNN
    if (TKelvin == 308.) {
        double pVec[100];
        int n = 0;
        for (int i = 0; i < 100; i++) {
            pVec[n++] = 6.8E6 + 2.0E5 * i;
        }

        for (int i = 0; i < 100; i++) {
            int nsol =  NicholsSolve(TKelvin, pVec[i], atmp, btmp, Vroot);
            printf("nsol = %d, p = %g, T = %g, v[0] = %g, v[1] %g, v[2] = %g\n", nsol, pVec[i], TKelvin, Vroot[0], Vroot[1], Vroot[2]);
        }
    }
#endif

    bool foundLiq = false;
    int m = 0;
    do {

        int nsol = NicholsSolve(TKelvin, pres, atmp, btmp, Vroot);

        // printf("nsol = %d\n", nsol);
        // printf("liquidVolEst start: T = %g , p = %g, a = %g, b = %g\n", TKelvin, pres, m_a_current, m_b_current);

        if (nsol == 1 || nsol == 2) {
            double pc = critPressure();
            if (pres > pc) {
                foundLiq = true;
            }
            pres *= 1.04;

        } else {
            foundLiq = true;
        }
    } while ((m < 100) && (!foundLiq));

#ifdef DONTUSE
    int i;
    double c;
    double vnew;
    double deltav;
    double sqt = sqrt(TKelvin);
    for (i = 0; i < 200; i++) {
        c = bCalc * bCalc + bCalc * GasConstant * TKelvin / pres - atmp / (pres * sqt);
        vnew = (1.0/c)*(v*v*v - GasConstant * TKelvin *v*v/pp - atmp * bCalc / (pres * sqt));
        deltav = vnew - v;
        if (deltav > v*0.2) {
            deltav = v * 0.2;
        } else if (deltav < - (v * 0.2)) {
            deltav = - v * 0.2;
        }
        v += deltav;
        if (fabs(deltav) < 1.0E-6 * v) {
            break;
        }
    }
    if (i > 30) {
        printf("liquidVolEst problem solve: T = %g , p = %g, a = %g, b = %g\n", TKelvin, pres, atmp, bCalc);
        printf("                                v final = %g\n", v);
    }
    if (fabs(deltav) > 1.0E-5 * v) {
        throw CanteraError("RedlichKwongMFTP::liquidVolEst(T = " + fp2str(TKelvin) + ", " + fp2str(pres) + ")",
                           "failed to converge");
    }
#else
    if (foundLiq) {
        v = Vroot[0];
        presGuess = pres;
    } else {
        v = -1.0;
    }
#endif
    //printf ("     RedlichKwongMFTP::liquidVolEst %g %g converged in %d its\n", TKelvin, pres, i);
    return v;
}
//====================================================================================================================
//  Calculates the density given the temperature and the pressure and a guess at the density.
/*
 * Note, below T_c, this is a multivalued function. We do not cross the vapor dome in this.
 * This is protected because it is called during setState_TP() routines. Infinite loops would result
 * if it were not protected.
 *
 *  -> why is this not const?
 *
 * parameters:
 *    @param TKelvin   Temperature in Kelvin
 *    @param pressure  Pressure in Pascals (Newton/m**2)
 *    @param phaseReqested     int representing the phase whose density we are requesting. If we put
 *                     a gas or liquid phase here, we will attempt to find a volume in that
 *                     part of the volume space, only, in this routine. A value of FLUID_UNDEFINED
 *                     means that we will accept anything.
 *
 *   @param rhoguess   Guessed density of the fluid. A value of -1.0 indicates that there
 *                     is no guessed density
 *
 *
 *  @return   We return the density of the fluid at the requested phase. If we have not found any
 *            acceptable density we return a -1. If we have found an accectable density at a
 *            different phase, we return a -2.
 */
doublereal RedlichKwongMFTP::densityCalc(doublereal TKelvin, doublereal presPa, int phaseRequested, doublereal rhoguess)
{

    /*
     *  It's necessary to set the temperature so that m_a_current is set correctly.
     */
    setTemperature(TKelvin);
    double tcrit = critTemperature();
    doublereal mmw = meanMolecularWeight();
    double densBase = 0.0;
    if (rhoguess == -1.0) {
        if (phaseRequested != FLUID_GAS) {
            if (TKelvin > tcrit) {
                rhoguess = presPa * mmw / (GasConstant * TKelvin);
            } else {
                if (phaseRequested == FLUID_GAS || phaseRequested == FLUID_SUPERCRIT) {
                    rhoguess = presPa * mmw / (GasConstant * TKelvin);
                } else if (phaseRequested >= FLUID_LIQUID_0) {
                    double lqvol = liquidVolEst(TKelvin, presPa);
                    rhoguess = mmw / lqvol;
                }
            }
        } else {
            /*
             * Assume the Gas phase initial guess, if nothing is
             * specified to the routine
             */
            rhoguess = presPa * mmw / (GasConstant * TKelvin);
        }

    }


    doublereal volguess = mmw / rhoguess;
    NSolns_ = NicholsSolve(TKelvin, presPa, m_a_current, m_b_current, Vroot_);

    doublereal molarVolLast = Vroot_[0];
    if (NSolns_ >= 2) {
        if (phaseRequested >= FLUID_LIQUID_0) {
            molarVolLast = Vroot_[0];
        } else if (phaseRequested == FLUID_GAS || phaseRequested == FLUID_SUPERCRIT) {
            molarVolLast = Vroot_[2];
        } else {
            if (volguess > Vroot_[1]) {
                molarVolLast = Vroot_[2];
            } else {
                molarVolLast = Vroot_[0];
            }
        }
    } else if (NSolns_ == 1) {
        if (phaseRequested == FLUID_GAS || phaseRequested == FLUID_SUPERCRIT || phaseRequested == FLUID_UNDEFINED) {
            molarVolLast = Vroot_[0];
        } else {
            //molarVolLast = Vroot_[0];
            //printf("DensityCalc(): Possible problem encountered\n");
            return -2.0;
        }
    } else if (NSolns_ == -1) {
        if (phaseRequested >= FLUID_LIQUID_0 || phaseRequested == FLUID_UNDEFINED ||  phaseRequested == FLUID_SUPERCRIT) {
            molarVolLast = Vroot_[0];
        } else if (TKelvin > tcrit) {
            molarVolLast = Vroot_[0];
        } else {
            //	molarVolLast = Vroot_[0];
            //printf("DensityCalc(): Possible problem encountered\n");
            return -2.0;
        }
    } else {
        molarVolLast = Vroot_[0];
        //printf("DensityCalc(): Possible problem encountered\n");
        return -1.0;
    }
    densBase = mmw / molarVolLast;
    return densBase;
}
//====================================================================================================================
// Return the value of the density at the liquid spinodal point (on the liquid side)
// for the current temperature.
/*
 * @return returns the density with units of kg m-3
 */
doublereal  RedlichKwongMFTP::densSpinodalLiquid() const
{
    if (NSolns_ != 3) {
        double dens = critDensity();
        return dens;
    }
    double vmax = Vroot_[1];
    double vmin = Vroot_[0];
    RootFind rf(fdpdv_);
    rf.setPrintLvl(10);
    rf.setTol(1.0E-5, 1.0E-10);
    rf.setFuncIsGenerallyDecreasing(true);

    double vbest = 0.5 * (Vroot_[0]+Vroot_[1]);
    double funcNeeded = 0.0;

    int status = rf.solve(vmin, vmax, 100, funcNeeded, &vbest);
    if (status != ROOTFIND_SUCCESS) {
        throw CanteraError("  RedlichKwongMFTP::densSpinodalLiquid() ", "didn't converge");
    }
    doublereal mmw = meanMolecularWeight();
    doublereal rho = mmw / vbest;
    return rho;
}
//====================================================================================================================
// Return the value of the density at the gas spinodal point (on the gas side)
// for the current temperature.
/*
 * @return returns the density with units of kg m-3
 */
doublereal RedlichKwongMFTP::densSpinodalGas() const
{
    if (NSolns_ != 3) {
        double dens = critDensity();
        return dens;
    }
    double vmax = Vroot_[2];
    double vmin = Vroot_[1];
    RootFind rf(fdpdv_);
    rf.setPrintLvl(10);
    rf.setTol(1.0E-5, 1.0E-10);
    rf.setFuncIsGenerallyIncreasing(true);

    double vbest = 0.5 * (Vroot_[1]+Vroot_[2]);
    double funcNeeded = 0.0;

    int status = rf.solve(vmin, vmax, 100, funcNeeded, &vbest);
    if (status != ROOTFIND_SUCCESS) {
        throw CanteraError("  RedlichKwongMFTP::densSpinodalGas() ", "didn't converge");
    }
    doublereal mmw = meanMolecularWeight();
    doublereal rho = mmw / vbest;
    return rho;
}
//====================================================================================================================
// Calculate the pressure given the temperature and the molar volume
/*
 *  Calculate the pressure given the temperature and the molar volume
 *
 * @param   TKelvin   temperature in kelvin
 * @param   molarVol  molar volume ( m3/kmol)
 *
 * @return  Returns the pressure.
 */
doublereal RedlichKwongMFTP::pressureCalc(doublereal TKelvin, doublereal molarVol) const
{
    doublereal sqt = sqrt(TKelvin);
    double pres = GasConstant * TKelvin / (molarVol - m_b_current)
                  - m_a_current / (sqt * molarVol * (molarVol + m_b_current));
    return pres;
}
//====================================================================================================================
// Calculate the pressure and the pressure derivative given the temperature and the molar volume
/*
 *  Temperature and mole number are held constant
 *
 * @param   TKelvin   temperature in kelvin
 * @param   molarVol  molar volume ( m3/kmol)
 *
 * @param   presCalc  Returns the pressure.
 *
 *  @return  Returns the derivative of the pressure wrt the molar volume
 */
doublereal  RedlichKwongMFTP::dpdVCalc(doublereal TKelvin, doublereal molarVol, doublereal& presCalc) const
{
    doublereal sqt = sqrt(TKelvin);
    presCalc = GasConstant * TKelvin / (molarVol - m_b_current)
               - m_a_current / (sqt * molarVol * (molarVol + m_b_current));

    doublereal vpb = molarVol + m_b_current;
    doublereal vmb = molarVol - m_b_current;
    doublereal dpdv = (- GasConstant * TKelvin / (vmb * vmb)
                       + m_a_current * (2 * molarVol + m_b_current) / (sqt * molarVol * molarVol * vpb * vpb));
    return dpdv;
}
//====================================================================================================================

void  RedlichKwongMFTP::pressureDerivatives() const
{
    doublereal TKelvin = temperature();
    doublereal mv = molarVolume();
    doublereal pres;

    dpdV_ = dpdVCalc(TKelvin, mv, pres);

    doublereal sqt = sqrt(TKelvin);
    doublereal vpb = mv + m_b_current;
    doublereal vmb = mv - m_b_current;
    doublereal dadt = da_dt();
    doublereal fac = dadt - m_a_current/(2.0 * TKelvin);

    dpdT_ = (GasConstant / (vmb) - fac / (sqt *  mv * vpb));
}
//====================================================================================================================
void RedlichKwongMFTP::updateMixingExpressions()
{
    updateAB();
}
//====================================================================================================================
void RedlichKwongMFTP::updateAB()
{
    double temp = temperature();
    int counter;
    if (m_formTempParam == 1) {
        for (int i = 0; i < m_kk; i++) {
            for (int j = 0; j < m_kk; j++) {
                counter = i * m_kk + j;
                a_vec_Curr_[counter] = a_coeff_vec(0,counter) + a_coeff_vec(1,counter) * temp;
            }
        }
    }

    m_b_current = 0.0;
    m_a_current = 0.0;
    for (int i = 0; i < m_kk; i++) {
        m_b_current += moleFractions_[i] * b_vec_Curr_[i];
        for (int j = 0; j < m_kk; j++) {
            m_a_current +=  a_vec_Curr_[i * m_kk + j] * moleFractions_[i] * moleFractions_[j];
        }
    }
}
//====================================================================================================================
void RedlichKwongMFTP::calculateAB(doublereal temp, doublereal& aCalc, doublereal& bCalc) const
{
    int counter;
    bCalc = 0.0;
    aCalc = 0.0;
    if (m_formTempParam == 1) {
        for (int i = 0; i < m_kk; i++) {
            bCalc += moleFractions_[i] * b_vec_Curr_[i];
            for (int j = 0; j < m_kk; j++) {
                counter = i * m_kk + j;
                doublereal a_vec_Curr = a_coeff_vec(0,counter) + a_coeff_vec(1,counter) * temp;
                aCalc +=  a_vec_Curr * moleFractions_[i] * moleFractions_[j];
            }
        }
    } else {
        for (int i = 0; i < m_kk; i++) {
            bCalc += moleFractions_[i] * b_vec_Curr_[i];
            for (int j = 0; j < m_kk; j++) {
                counter = i * m_kk + j;
                doublereal a_vec_Curr = a_coeff_vec(0,counter);
                aCalc +=  a_vec_Curr * moleFractions_[i] * moleFractions_[j];
            }
        }
    }
}
//====================================================================================================================
doublereal RedlichKwongMFTP::da_dt() const
{

    doublereal dadT = 0.0;
    if (m_formTempParam == 1) {
        for (int i = 0; i < m_kk; i++) {
            for (int j = 0; j < m_kk; j++) {
                int counter = i * m_kk + j;
                dadT+=  a_coeff_vec(1,counter) * moleFractions_[i] * moleFractions_[j];
            }
        }
    }
    return dadT;
}
//====================================================================================================================
void RedlichKwongMFTP::calcCriticalConditions(doublereal a, doublereal b, doublereal a0_coeff, doublereal aT_coeff,
        doublereal& pc, doublereal& tc, doublereal& vc) const
{
    if (m_formTempParam != 0) {
        a = a0_coeff;
    }
    if (b <= 0.0) {
        tc = 1000000.;
        pc = 1.0E13;
        vc = omega_vc * GasConstant * tc / pc;
        return;
    }
    if (a <= 0.0) {
        tc = 0.0;
        pc = 0.0;
        vc = 2.0 * b;
        return;
    }
    double tmp = a * omega_b / (b * omega_a * GasConstant);
    double pp = 2./3.;
    doublereal sqrttc, f, dfdt, deltatc;

    if (m_formTempParam == 0) {

        tc = pow(tmp, pp);
    } else {
        tc = pow(tmp, pp);
        for (int j = 0; j < 10; j++) {
            sqrttc = sqrt(tc);
            f =  omega_a * b * GasConstant * tc * sqrttc / omega_b - aT_coeff * tc - a0_coeff;
            dfdt = 1.5 * omega_a * b * GasConstant * sqrttc / omega_b - aT_coeff;
            deltatc = - f / dfdt;
            tc += deltatc;
        }
        if (deltatc > 0.1) {
            throw CanteraError("RedlichKwongMFTP::calcCriticalConditions", "didn't converge");
        }
    }

    pc = omega_b * GasConstant * tc / b;
    vc = omega_vc * GasConstant * tc / pc;
}

//====================================================================================================================
// Solve the cubic equation of state
/*
 * The R-K equation of state may be solved via the following formula
 *
 *     V**3 - V**2(RT/P)  - V(RTb/P - a/(P T**.5) + b*b) - (a b / (P T**.5)) = 0
 *

 * Returns the number of solutions found. If it only finds the liquid branch solution, it will return a -1 or a -2
 * instead of 1 or 2.  If it returns 0, then there is an error.
 *
 */
int RedlichKwongMFTP::NicholsSolve(double TKelvin, double pres, doublereal a, doublereal b,
                                   doublereal Vroot[3]) const
{
    Vroot[0] = 0.0;
    Vroot[1] = 0.0;
    Vroot[2] = 0.0;
    int nTurningPoints;
    bool lotsOfNumError = false;
    doublereal Vturn[2];
    if (TKelvin <= 0.0) {
        throw CanteraError("RedlichKwongMFTP::NicholsSolve()",  "neg temperature");
    }
    /*
     *  Derive the coefficients of the cubic polynomial to solve.
     */
    doublereal an = 1.0;
    doublereal bn = - GasConstant * TKelvin / pres;
    doublereal sqt = sqrt(TKelvin);
    doublereal cn = - (GasConstant * TKelvin * b / pres - a/(pres * sqt) + b * b);
    doublereal dn = - (a * b / (pres * sqt));

    double tmp = a * omega_b / (b * omega_a * GasConstant);
    double pp = 2./3.;
    double tc =  pow(tmp, pp);
    double pc = omega_b * GasConstant * tc / b;
    double   vc = omega_vc * GasConstant * tc / pc;
    // Derive the center of the cubic, x_N
    doublereal xN = - bn /(3 * an);


    // Derive the value of delta**2. This is a key quantity that determines the number of turning points
    doublereal delta2 = (bn * bn - 3 * an * cn) / (9 * an * an);
    doublereal delta = 0.0;

    // Calculate a couple of ratios
    doublereal ratio1 = 3.0 * an * cn / (bn * bn);
    doublereal ratio2 = pres * b / (GasConstant * TKelvin);
    if (fabs(ratio1) < 1.0E-7) {
        //printf("NicholsSolve(): Alternative solution (p = %g T = %g)\n", pres, TKelvin);
        doublereal ratio3 = a / (GasConstant * sqt) * pres / (GasConstant * TKelvin);
        if (fabs(ratio2) < 1.0E-5 && fabs(ratio3) < 1.0E-5) {
            doublereal z = 1.0;
            for (int i = 0; i < 10; i++) {
                doublereal  znew = z / (z - ratio2) - ratio3 / (z + ratio1);
                doublereal deltaz = znew - z;
                z = znew;
                if (fabs(deltaz) < 1.0E-14) {
                    break;
                }
            }
            doublereal v = z * GasConstant * TKelvin / pres;
            Vroot[0] = v;
            return 1;
        }
    }


    int nSolnValues;
    nTurningPoints = 2;

#ifdef PRINTPV
    double V[100];
    int n = 0;
    for (int i = 0; i < 90; i++) {
        V[n++] = 0.030 + 0.005 * i;
    }
    double p1, presCalc;
    for (int i = 0; i < n; i++) {
        p1 = dpdVCalc(TKelvin, V[i], presCalc);
        printf(" %13.5g %13.5g %13.5g \n", V[i], presCalc , p1);
    }
#endif

    double h2 = 4. * an * an * delta2 * delta2 * delta2;
    if (delta2 == 0.0) {
        nTurningPoints = 1;
        Vturn[0] = xN;
        Vturn[1] = xN;
    } else if (delta2 < 0.0) {
        nTurningPoints = 0;
        Vturn[0] = xN;
        Vturn[1] = xN;
    } else {
        delta = sqrt(delta2);
        Vturn[0] = xN - delta;
        Vturn[1] = xN + delta;
#ifdef PRINTPV
        double presCalc;
        double p1 = dpdVCalc(TKelvin, Vturn[0], presCalc);

        double p2 = dpdVCalc(TKelvin, Vturn[1], presCalc);

        printf("p1 = %g p2 = %g \n", p1, p2);
        p1 = dpdVCalc(TKelvin, 0.9*Vturn[0], presCalc);
        printf("0.9 p1 = %g \n", p1);
#endif
    }

    doublereal h = 2.0 * an * delta * delta2;

    doublereal yN = 2.0 * bn * bn * bn / (27.0 * an * an) - bn * cn / (3.0 * an) + dn;

    doublereal desc = yN * yN - h2;

    if (fabs(fabs(h) - fabs(yN)) < 1.0E-10) {
        if (desc != 0.0) {
            // this is for getting to other cases
            printf("NicholsSolve(): numerical issues\n");
            throw CanteraError("NicholsSolve()", "numerical issues");
        }
        desc = 0.0;
    }

    if (desc < 0.0) {
        nSolnValues = 3;
    } else if (desc == 0.0) {
        nSolnValues = 2;
        // We are here as p goes to zero.
        //  double hleft = 3.0 * an * cn / (bn * bn);
        //double ynleft = 9.0 * an * cn / (2.0 * bn * bn) - 27.0 * an * an * dn / (2.0 * bn * bn * bn);
        //printf("hleft = %g , ynleft = %g\n", -3. / 2. * hleft, -ynleft);
        //double h2left = - 3 *  hleft + 3 * hleft * hleft - hleft * hleft * hleft;
        //double y2left = - 2.0 * ynleft + ynleft * ynleft;
        //printf("h2left = %g , yn2left = %g\n", h2left, y2left);

    } else if (desc > 0.0) {
        nSolnValues = 1;
    }

    /*
     *  One real root -> have to determine whether gas or liquid is the root
     */
    if (desc > 0.0) {
        doublereal tmpD = sqrt(desc);
        doublereal tmp1 = (- yN + tmpD) / (2.0 * an);
        doublereal sgn1 = 1.0;
        if (tmp1 < 0.0) {
            sgn1 = -1.0;
            tmp1 = -tmp1;
        }
        doublereal tmp2 = (- yN - tmpD) / (2.0 * an);
        doublereal sgn2 = 1.0;
        if (tmp2 < 0.0) {
            sgn2 = -1.0;
            tmp2 = -tmp2;
        }
        doublereal p1 = pow(tmp1, 1./3.);
        doublereal p2 = pow(tmp2, 1./3.);

        doublereal alpha = xN + sgn1 * p1 + sgn2 * p2;
        Vroot[0] = alpha;
        Vroot[1] = 0.0;
        Vroot[2] = 0.0;

        double tmp = an *  Vroot[0] * Vroot[0] * Vroot[0] + bn * Vroot[0] * Vroot[0] + cn  * Vroot[0] + dn;
        if (fabs(tmp) > 1.0E-4) {
            lotsOfNumError = true;
        }

    } else if (desc < 0.0) {
        doublereal tmp = - yN/h;

        doublereal val = acos(tmp);
        doublereal theta = val / 3.0;

        doublereal oo = 2. * Cantera::Pi / 3.;
        doublereal alpha = xN + 2. * delta * cos(theta);

        doublereal beta = xN + 2. * delta * cos(theta + oo);

        doublereal gamma = xN + 2. * delta * cos(theta + 2.0 * oo);


        Vroot[0] = beta;
        Vroot[1] = gamma;
        Vroot[2] = alpha;

        for (int i = 0; i < 3; i++) {
            double tmp = an *  Vroot[i] * Vroot[i] * Vroot[i] + bn * Vroot[i] * Vroot[i] + cn  * Vroot[i] + dn;
            if (fabs(tmp) > 1.0E-4) {
                lotsOfNumError = true;
                for (int j = 0; j < 3; j++) {
                    if (j != i) {
                        if (fabs(Vroot[i] - Vroot[j]) < 1.0E-4 * (fabs(Vroot[i]) + fabs(Vroot[j]))) {
                            writelog("RedlichKwongMFTP::NicholsSolve(T = " + fp2str(TKelvin) + ", p = " +
                                     fp2str(pres) + "): WARNING roots have merged: " +
                                     fp2str(Vroot[i]) + ", " + fp2str(Vroot[j]));
                            writelogendl();
                        }
                    }
                }
            }
        }
    } else if (desc == 0.0) {
        if (yN == 0.0 && h == 0.0) {
            Vroot[0] = xN;
            Vroot[1] = xN;
            Vroot[2] = xN;
        } else {
            // need to figure out whether delta is pos or neg
            if (yN > 0.0) {
                double tmp = pow(yN/(2*an), 1./3.);
                if (fabs(tmp - delta) > 1.0E-9) {
                    throw CanteraError("RedlichKwongMFTP::NicholsSolve()", "unexpected");
                }
                Vroot[1] = xN + delta;
                Vroot[0] = xN - 2.0*delta;  // liquid phase root
            } else {
                double tmp = pow(yN/(2*an), 1./3.);
                if (fabs(tmp - delta) > 1.0E-9) {
                    throw CanteraError("RedlichKwongMFTP::NicholsSolve()", "unexpected");
                }
                delta = -delta;
                Vroot[0] = xN + delta;
                Vroot[1] = xN - 2.0*delta;  // gas phase root
            }
        }
        for (int i = 0; i < 2; i++) {
            double tmp = an *  Vroot[i] * Vroot[i] * Vroot[i] + bn * Vroot[i] * Vroot[i] + cn  * Vroot[i] + dn;
            if (fabs(tmp) > 1.0E-4) {
                lotsOfNumError = true;
            }
        }
    }

    /*
     * Unfortunately, there is a heavy amount of roundoff error due to bad conditioning in this
     */
    double res, dresdV;;
    for (int i = 0; i < nSolnValues; i++) {
        for (int n = 0; n < 20; n++) {
            res = an *  Vroot[i] * Vroot[i] * Vroot[i] + bn * Vroot[i] * Vroot[i] + cn  * Vroot[i] + dn;
            if (fabs(res) < 1.0E-14) {
                break;
            }
            dresdV = 3.0 * an *  Vroot[i] * Vroot[i] + 2.0 * bn * Vroot[i] + cn;
            double del = - res / dresdV;

            Vroot[i] += del;
            if (fabs(del) / (fabs(Vroot[i]) + fabs(del)) < 1.0E-14) {
                break;
            }
            double res2 = an *  Vroot[i] * Vroot[i] * Vroot[i] + bn * Vroot[i] * Vroot[i] + cn  * Vroot[i] + dn;
            if (fabs(res2) < fabs(res)) {
                continue;
            } else {
                Vroot[i] -= del;
                Vroot[i] += 0.1 * del;
            }
        }
        if ((fabs(res) > 1.0E-14) && (fabs(res) > 1.0E-14 * fabs(dresdV) * fabs(Vroot[i]))) {
            writelog("RedlichKwongMFTP::NicholsSolve(T = " + fp2str(TKelvin) + ", p = " +
                     fp2str(pres) + "): WARNING root didn't converge V = " + fp2str(Vroot[i]));
            writelogendl();
        }
    }

    if (nSolnValues == 1) {
        if (TKelvin > tc) {
            if (Vroot[0] < vc) {
                nSolnValues = -1;
            }
        } else {
            if (Vroot[0] < xN) {
                nSolnValues = -1;
            }
        }

    } else {
        if (nSolnValues == 2) {
            if (delta > 0.0) {
                nSolnValues = -2;
            }
        }
    }
    // writelog("RedlichKwongMFTP::NicholsSolve(T = " + fp2str(TKelvin) + ", p = " + 	 fp2str(pres) + "): finished");
    // writelogendl();
    return nSolnValues;
}


#endif
}


