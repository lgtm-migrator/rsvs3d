#include <Eigen>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "RSVScalc.hpp"
#include "RSVScalctools.hpp"
#include "RSVSmath.hpp"
#include "matrixtools.hpp"
#include "rsvsutils.hpp"
#include "snake.hpp"
#include "triangulate.hpp"
#include "vectorarray.hpp"
#include "warning.hpp"

using namespace std;
using namespace Eigen;
using namespace rsvs3d::constants;

/*
Implementation of the core calculation functions of
RSVScalc, this is done to reduce the size of the objects and
keep compilation time manageable.

*/

//==========================================
// Core class functions
//==========================================

void RSVScalc::CalcTriangle(const triangle &triIn, const triangulation &triRSVS, bool isObj, bool isConstr,
                            bool isDeriv)
{
    int ii, jj, nj, nCellTarg;
    int nDvAct;
    SurfCentroid centreCalc;
    Volume VolumeCalc;
    Area AreaCalc;
    vector<int> subTempVec;
    HashedVector<int, int> dvListMap;
    grid::coordlist veccoord;
    MatrixXd HPos, dPos;
    MatrixXd HVal, dVal;
    MatrixXd dConstrPart, HConstrPart, HObjPart;
    MatrixXd dObjPart;
    double constrPart, objPart;
    ArrayVec<double> *HValpnt = NULL, *dValpnt = NULL;
    double *retVal;

    auto clock1 = rsvs3d::TimeStamp(NULL, 0);
    // Prepare calculation steps
    veccoord = TrianglePointerCoordinates(triIn, triRSVS);
    dvListMap = TriangleActiveDesignVariables(triIn, triRSVS, this->dvMap, this->useSurfCentreDeriv);
    TrianglePositionDerivatives(triIn, triRSVS, dvListMap, dPos, HPos, this->useSurfCentreDeriv,
                                this->useSurfCentreHessian);

    auto clock2 = rsvs3d::TimeStamp(NULL, clock1);
    this->timer1 += (clock2 - clock1);
    clock1 = clock2;
    // Total

    // Constr and objective
    nDvAct = dvListMap.vec.size();
    VolumeCalc.assign(veccoord[0], veccoord[1], veccoord[2]);
    AreaCalc.assign(veccoord[0], veccoord[1], veccoord[2]);

    HVal.setZero(9, 9);
    dVal.setZero(1, 9);
    dConstrPart.setZero(1, nDvAct);
    HConstrPart.setZero(nDvAct, nDvAct);

    if (isConstr)
    {
        VolumeCalc.Calc();
        VolumeCalc.ReturnDatPoint(&retVal, &dValpnt, &HValpnt);
        ArrayVec2MatrixXd(*HValpnt, HVal);
        ArrayVec2MatrixXd(*dValpnt, dVal);
        Deriv1stChainScalar(dVal, dPos, dConstrPart);
        Deriv2ndChainScalar(dVal, dPos, HVal, HPos, HConstrPart);
        // HConstrPart = dConstrPart.transpose()*dConstrPart;

        constrPart = *retVal;
        // Assign Constraint
        // and constraint derivative
        // and Hessian
        nCellTarg = triIn.connec.celltarg.size();
        for (ii = 0; ii < nCellTarg; ++ii)
        {
            subTempVec = this->constrMap.findall(triIn.connec.celltarg[ii]);
            nj = subTempVec.size();
            for (jj = 0; jj < nj; ++jj)
            {
                if (subTempVec[jj] != __notfound)
                {
                    this->constr[subTempVec[jj]] += triIn.connec.constrinfluence[ii] * constrPart;
                    if (isDeriv)
                    {
                        if (this->UseFullMath())
                        {
                            AssignConstraintDerivativesFullMath(*this, triIn, dvListMap, dConstrPart, HConstrPart,
                                                                subTempVec[jj], ii);
                        }
                        else
                        {
                            AssignConstraintDerivativesSparseMath(*this, triIn, dvListMap, dConstrPart, HConstrPart,
                                                                  subTempVec[jj], ii);
                        }
                    }
                }
                else
                {
                    this->falseaccess++;
                }
            }
        }
    }
    clock2 = rsvs3d::TimeStamp(NULL, clock1);
    this->timer2 += (clock2 - clock1);
    clock1 = clock2;

    HVal.setZero(9, 9);
    dVal.setZero(1, 9);
    dObjPart.setZero(1, nDvAct);
    HObjPart.setZero(nDvAct, nDvAct);

    if (isObj)
    {
        AreaCalc.Calc();
        AreaCalc.ReturnDatPoint(&retVal, &dValpnt, &HValpnt);
        ArrayVec2MatrixXd(*HValpnt, HVal);
        ArrayVec2MatrixXd(*dValpnt, dVal);
        Deriv1stChainScalar(dVal, dPos, dObjPart);
        Deriv2ndChainScalar(dVal, dPos, HVal, HPos, HObjPart);
        // HObjPart = dObjPart.transpose()*dObjPart;
        objPart = *retVal;

        // Assign to main part of the object
        // assign objective function
        this->obj += objPart;
        // Assign objective derivative
        if (isDeriv)
        {
            for (ii = 0; ii < nDvAct; ++ii)
            {
                this->dObj[this->dvMap.find(dvListMap.vec[ii])] += dObjPart(0, ii);
                if (this->UseFullMath())
                {
                    for (jj = 0; jj < nDvAct; ++jj)
                    {
                        this->HObj(dvMap.find(dvListMap.vec[jj]), this->dvMap.find(dvListMap.vec[ii])) +=
                            HObjPart(jj, ii);
                    }
                }
                else
                {
                    for (jj = 0; jj < nDvAct; ++jj)
                    {
                        if (!rsvs3d::utils::IsAproxEqual(HObjPart(jj, ii), 0.0))
                        {
                            this->HObj_sparse.coeffRef(dvMap.find(dvListMap.vec[jj]),
                                                       this->dvMap.find(dvListMap.vec[ii])) += HObjPart(jj, ii);
                        }
                    }
                }
            }
        }
    }
    clock2 = rsvs3d::TimeStamp(NULL, clock1);
    this->timer3 += (clock2 - clock1);
    clock1 = clock2;
    // Update active lists of design variables
    for (ii = 0; ii < nDvAct; ++ii)
    {
        this->isDvAct.at(dvMap.find(dvListMap.vec[ii])) = true;
    }
    if (nDvAct > 0)
    {
        nCellTarg = triIn.connec.celltarg.size();
        for (ii = 0; ii < nCellTarg; ++ii)
        {
            subTempVec = constrMap.findall(triIn.connec.celltarg[ii]);
            nj = subTempVec.size();
            for (jj = 0; jj < nj; ++jj)
            {
                if (subTempVec[jj] != __notfound)
                {
                    this->isConstrAct.at(subTempVec[jj]) = true;
                }
            }
        }
    }

    // Assign Objective Hessian

    // Assign Constraint Hessian
}

void RSVScalc::CalcTriangleFD(const triangle &triIn, const triangulation &triRSVS, bool isObj, bool isConstr,
                              bool isDeriv)
{
    /*Same as calctriangle but the volume derivative is calculated using a
    Finite Difference.

    helps isolate errors in RSVSmath.
    */

    int ii, jj, nj, nCellTarg;
    int nDvAct;
    SurfCentroid centreCalc;
    Volume VolumeCalc;
    Area AreaCalc;
    vector<int> dvList, subTempVec;
    HashedVector<int, int> dvListMap;
    grid::coordlist veccoord;
    MatrixXd HPos, dPos;
    MatrixXd HVal, dVal;
    MatrixXd dConstrPart, HConstrPart, HObjPart;
    MatrixXd dObjPart;
    double constrPart, objPart;
    ArrayVec<double> *HValpnt = NULL, *dValpnt = NULL;
    double *retVal;

    // Prepare calculation steps
    veccoord = TrianglePointerCoordinates(triIn, triRSVS);
    dvListMap = TriangleActiveDesignVariables(triIn, triRSVS, this->dvMap, this->useSurfCentreDeriv);
    TrianglePositionDerivatives(triIn, triRSVS, dvListMap, dPos, HPos, this->useSurfCentreDeriv,
                                this->useSurfCentreHessian);

    // Constr and objective
    nDvAct = dvListMap.vec.size();
    VolumeCalc.assign(veccoord[0], veccoord[1], veccoord[2]);
    AreaCalc.assign(veccoord[0], veccoord[1], veccoord[2]);

    HVal.setZero(9, 9);
    dVal.setZero(1, 9);
    dConstrPart.setZero(1, nDvAct);
    HConstrPart.setZero(nDvAct, nDvAct);

    if (isConstr)
    {
        VolumeCalc.CalcFD();
        VolumeCalc.ReturnDatPoint(&retVal, &dValpnt, &HValpnt);
        ArrayVec2MatrixXd(*HValpnt, HVal);
        ArrayVec2MatrixXd(*dValpnt, dVal);
        Deriv1stChainScalar(dVal, dPos, dConstrPart);
        Deriv2ndChainScalar(dVal, dPos, HVal, HPos, HConstrPart);
        // HConstrPart = dConstrPart.transpose()*dConstrPart;
        constrPart = *retVal;
        // Assign Constraint
        // and constraint derivative
        // and Hessian
        nCellTarg = triIn.connec.celltarg.size();
        for (ii = 0; ii < nCellTarg; ++ii)
        {
            subTempVec = this->constrMap.findall(triIn.connec.celltarg[ii]);
            nj = subTempVec.size();
            for (jj = 0; jj < nj; ++jj)
            {
                if (subTempVec[jj] != __notfound)
                {
                    this->constr[subTempVec[jj]] += triIn.connec.constrinfluence[ii] * constrPart;
                    if (isDeriv)
                    {
                        if (this->UseFullMath())
                        {
                            AssignConstraintDerivativesFullMath(*this, triIn, dvListMap, dConstrPart, HConstrPart,
                                                                subTempVec[jj], ii);
                        }
                        else
                        {
                            AssignConstraintDerivativesSparseMath(*this, triIn, dvListMap, dConstrPart, HConstrPart,
                                                                  subTempVec[jj], ii);
                        }
                    }
                }
                else
                {
                    this->falseaccess++;
                }
            }
        }
    }

    HVal.setZero(9, 9);
    dVal.setZero(1, 9);
    dObjPart.setZero(1, nDvAct);
    HObjPart.setZero(nDvAct, nDvAct);

    if (isObj)
    {
        AreaCalc.Calc();
        AreaCalc.ReturnDatPoint(&retVal, &dValpnt, &HValpnt);
        ArrayVec2MatrixXd(*HValpnt, HVal);
        ArrayVec2MatrixXd(*dValpnt, dVal);
        Deriv1stChainScalar(dVal, dPos, dObjPart);
        Deriv2ndChainScalar(dVal, dPos, HVal, HPos, HObjPart);
        // HObjPart = dObjPart.transpose()*dObjPart;
        objPart = *retVal;

        // Assign to main part of the object
        // assign objective function
        this->obj += objPart;
        // Assign objective derivative
        if (isDeriv)
        {
            for (ii = 0; ii < nDvAct; ++ii)
            {
                this->dObj[this->dvMap.find(dvListMap.vec[ii])] += dObjPart(0, ii);
                if (this->UseFullMath())
                {
                    for (jj = 0; jj < nDvAct; ++jj)
                    {
                        this->HObj(dvMap.find(dvListMap.vec[jj]), this->dvMap.find(dvListMap.vec[ii])) +=
                            HObjPart(jj, ii);
                    }
                }
                else
                {
                    for (jj = 0; jj < nDvAct; ++jj)
                    {
                        this->HObj_sparse.coeffRef(dvMap.find(dvListMap.vec[jj]),
                                                   this->dvMap.find(dvListMap.vec[ii])) += HObjPart(jj, ii);
                    }
                }
            }
        }
    }
    // Update active lists of design variables
    for (ii = 0; ii < nDvAct; ++ii)
    {
        this->isDvAct.at(dvMap.find(dvListMap.vec[ii])) = true;
    }
    if (nDvAct > 0)
    {
        nCellTarg = triIn.connec.celltarg.size();
        for (ii = 0; ii < nCellTarg; ++ii)
        {
            subTempVec = constrMap.findall(triIn.connec.celltarg[ii]);
            nj = subTempVec.size();
            for (jj = 0; jj < nj; ++jj)
            {
                if (subTempVec[jj] != __notfound)
                {
                    this->isConstrAct.at(subTempVec[jj]) = true;
                }
            }
        }
    }
    // Assign Objective Hessian

    // Assign Constraint Hessian
}

void RSVScalc::CalcTriangleDirectVolume(const triangle &triIn, const triangulation &triRSVS, bool isObj, bool isConstr,
                                        bool isDeriv)
{
    /*Same as calctriangle but the volume derivative is calculated without
    intermediate steps

    helps isolate errors on dPos.
    */

    int ii, ni, jj, nj, nCellTarg;
    int subTemp, nDvAct;
    SurfCentroid centreCalc;
    Volume2 VolumeCalc2;
    Volume VolumeCalc;
    Area AreaCalc;
    vector<int> dvList, subTempVec, dvOrder;
    HashedVector<int, int> dvListMap;
    grid::coordlist veccoord, veccoordvol;
    vector<double> dvec;
    MatrixXd HPos, dPos;
    MatrixXd HVal, dVal;
    MatrixXd dConstrPart, HConstrPart, HObjPart;
    MatrixXd dObjPart;
    double constrPart, objPart;
    ArrayVec<double> *HValpnt = NULL, *dValpnt = NULL;
    ArrayVec<double> *HValpnt2 = NULL, *dValpnt2 = NULL;
    double *retVal = NULL;
    double *retVal2 = NULL;

    // Prepare calculation steps
    veccoord = TrianglePointerCoordinates(triIn, triRSVS);
    dvListMap = TriangleActiveDesignVariables(triIn, triRSVS, this->dvMap, this->useSurfCentreDeriv);
    TrianglePositionDerivatives(triIn, triRSVS, dvListMap, dPos, HPos, this->useSurfCentreDeriv,
                                this->useSurfCentreHessian);

    // Constr and objective
    nDvAct = dvListMap.vec.size();
    VolumeCalc.assign(veccoord[0], veccoord[1], veccoord[2]);
    AreaCalc.assign(veccoord[0], veccoord[1], veccoord[2]);

    HVal.setZero(9, 9);
    dVal.setZero(1, 9);
    dConstrPart.setZero(1, nDvAct);
    HConstrPart.setZero(nDvAct, nDvAct);

    // Volume calc assignement
    dvOrder.assign(3, 0);
    ni = 3;
    for (ii = 0; ii < ni; ++ii)
    {
        if (triIn.pointtype[ii] == meshtypes::mesh)
        {
            dvec[ii] = 0;
            veccoordvol[1 + ii] = &(triRSVS.meshDep->verts.isearch(triIn.pointind[ii])->coord);
            veccoordvol[1 + 3 + ii] = &(triRSVS.meshDep->verts.isearch(triIn.pointind[ii])->coord);
        }
        else if (triIn.pointtype[ii] == meshtypes::snake)
        {
            subTemp = triRSVS.snakeDep->snakeconn.verts.find(triIn.pointind[ii]);
            dvOrder[ii] = triIn.pointind[ii];
            dvec[ii] = triRSVS.snakeDep->snaxs(subTemp)->d;
            veccoordvol[1 + ii] = &(triRSVS.meshDep->verts.isearch(triRSVS.snakeDep->snaxs(subTemp)->fromvert)->coord);
            veccoordvol[1 + ii + 3] =
                &(triRSVS.meshDep->verts.isearch(triRSVS.snakeDep->snaxs(subTemp)->tovert)->coord);
        }
        else if (triIn.pointtype[ii] == meshtypes::triangulation)
        {
            dvec[ii] = 0; // dvec used as a pointer
            veccoordvol[1 + ii] = (triRSVS.trivert.isearch(triIn.pointind[ii])->coord.retPtr());
            veccoordvol[1 + ii + 3] = (triRSVS.trivert.isearch(triIn.pointind[ii])->coord.retPtr());
        }
    }

    // Constr and objective
    VolumeCalc2.assign(veccoordvol); /// <-----Change assignement
    VolumeCalc.assign(veccoord[0], veccoord[1],
                      veccoord[2]); /// <-----Change assignement
    AreaCalc.assign(veccoord[0], veccoord[1], veccoord[2]);

    HVal.setZero(9, 9);
    dVal.setZero(1, 9);
    dConstrPart.setZero(1, nDvAct);
    HConstrPart.setZero(nDvAct, nDvAct);

    if (isConstr)
    {
        VolumeCalc2.Calc();
        VolumeCalc.Calc();
        VolumeCalc.ReturnDatPoint(&retVal, &dValpnt, &HValpnt);
        VolumeCalc2.ReturnDatPoint(&retVal2, &dValpnt2, &HValpnt2);
        ArrayVec2MatrixXd(*HValpnt2, HConstrPart);
        ArrayVec2MatrixXd(*dValpnt2, dConstrPart);

        if (fabs(*retVal - *retVal2) > 1e-10)
        {
            this->falseaccess++;
        }
        constrPart = *retVal2;

        if (dvListMap.find(1044) != __notfound && isDeriv)
        {
            cout << endl;
            DisplayVector(dvOrder);
            cout << endl;
            PrintMatrix(dConstrPart);
            cout << endl;
        }

        // Assign Constraint
        // and constraint derivative
        // and Hessian
        nCellTarg = triIn.connec.celltarg.size();
        for (ii = 0; ii < nCellTarg; ++ii)
        {
            subTempVec = this->constrMap.findall(triIn.connec.celltarg[ii]);
            nj = subTempVec.size();
            for (jj = 0; jj < nj; ++jj)
            {
                if (subTempVec[jj] != __notfound)
                {
                    this->constr[subTempVec[jj]] += triIn.connec.constrinfluence[ii] * constrPart;
                    if (isDeriv)
                    {
                        if (this->UseFullMath())
                        {
                            AssignConstraintDerivativesFullMath(*this, triIn, dvListMap, dConstrPart, HConstrPart,
                                                                subTempVec[jj], ii);
                        }
                        else
                        {
                            AssignConstraintDerivativesSparseMath(*this, triIn, dvListMap, dConstrPart, HConstrPart,
                                                                  subTempVec[jj], ii);
                        }
                    }
                }
                else
                {
                    this->falseaccess++;
                }
            }
        }
    }

    HVal.setZero(9, 9);
    dVal.setZero(1, 9);
    dObjPart.setZero(1, nDvAct);
    HObjPart.setZero(nDvAct, nDvAct);

    if (isObj)
    {
        AreaCalc.Calc();
        AreaCalc.ReturnDatPoint(&retVal, &dValpnt, &HValpnt);
        ArrayVec2MatrixXd(*HValpnt, HVal);
        ArrayVec2MatrixXd(*dValpnt, dVal);
        Deriv1stChainScalar(dVal, dPos, dObjPart);
        Deriv2ndChainScalar(dVal, dPos, HVal, HPos, HObjPart);
        objPart = *retVal;

        // Assign to main part of the object
        // assign objective function
        this->obj += objPart;
        // Assign objective derivative
        if (isDeriv)
        {
            for (ii = 0; ii < nDvAct; ++ii)
            {
                this->dObj[this->dvMap.find(dvListMap.vec[ii])] += dObjPart(0, ii);
                if (this->UseFullMath())
                {
                    for (jj = 0; jj < nDvAct; ++jj)
                    {
                        this->HObj(dvMap.find(dvListMap.vec[jj]), this->dvMap.find(dvListMap.vec[ii])) +=
                            HObjPart(jj, ii);
                    }
                }
                else
                {
                    for (jj = 0; jj < nDvAct; ++jj)
                    {
                        this->HObj_sparse.coeffRef(dvMap.find(dvListMap.vec[jj]),
                                                   this->dvMap.find(dvListMap.vec[ii])) += HObjPart(jj, ii);
                    }
                }
            }
        }
    }
    // Update active lists of design variables
    for (ii = 0; ii < nDvAct; ++ii)
    {
        this->isDvAct.at(dvMap.find(dvListMap.vec[ii])) = true;
    }
    if (nDvAct > 0)
    {
        nCellTarg = triIn.connec.celltarg.size();
        for (ii = 0; ii < nCellTarg; ++ii)
        {
            subTempVec = constrMap.findall(triIn.connec.celltarg[ii]);
            nj = subTempVec.size();
            for (jj = 0; jj < nj; ++jj)
            {
                if (subTempVec[jj] != __notfound)
                {
                    this->isConstrAct.at(subTempVec[jj]) = true;
                }
            }
        }
    }

    // Assign Objective Hessian

    // Assign Constraint Hessian
}

// =====================================
//
//
// Flag change is temporary
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void RSVScalc::CalcTriangleEdgeLength(const triangle &triIn, const triangulation &triRSVS, bool isObj, bool isConstr,
                                      bool isDeriv)
{
#pragma GCC diagnostic pop

    int ii, ni, jj, nj, nCellTarg;
    int subTemp, subTemp1, subTemp2, subTemp3, nDvAct;
    SurfCentroid centreCalc;
    LengthEdge EdgeCalc;
    Area AreaCalc;
    vector<int> dvList, subTempVec;
    HashedVector<int, int> dvListMap;
    grid::coordlist veccoord;
    MatrixXd HPos, dPos;
    MatrixXd HVal, dVal, HVal2, dVal2;
    MatrixXd dConstrPart, HConstrPart, HObjPart;
    MatrixXd dObjPart;
    double objPart;
    ArrayVec<double> *HValpnt = NULL, *dValpnt = NULL;
    double *retVal;

    veccoord.reserve(3);
    ni = 3;
    for (ii = 0; ii < ni; ++ii)
    {
        if (triIn.pointtype[ii] == meshtypes::mesh)
        {
            veccoord.push_back(&(triRSVS.meshDep->verts.isearch(triIn.pointind[ii])->coord));
        }
        else if (triIn.pointtype[ii] == meshtypes::snake)
        {
            veccoord.push_back(&(triRSVS.snakeDep->snakeconn.verts.isearch(triIn.pointind[ii])->coord));
        }
        else if (triIn.pointtype[ii] == meshtypes::triangulation)
        {
            veccoord.push_back((triRSVS.trivert.isearch(triIn.pointind[ii])->coord.retPtr()));
        }
    }

    // Constr and objective
    // CoordFunc.assign(int pRepI,vector<double> &pRep);
    AreaCalc.assign(veccoord[0], veccoord[1], veccoord[2]);

    // Active DV lists

    for (ii = 0; ii < ni; ++ii)
    {
        if (triIn.pointtype[ii] == meshtypes::snake && this->dvMap.find(triIn.pointind[ii]) != __notfound)
        {
            dvList.push_back(triIn.pointind[ii]);
        }
        else if (triIn.pointtype[ii] == meshtypes::triangulation && false)
        {
            subTemp = triRSVS.trivert.find(triIn.pointind[ii]);
            nj = triRSVS.trisurf(subTemp)->indvert.size();
            for (jj = 0; jj < nj; ++jj)
            {
                if (triRSVS.trisurf(subTemp)->typevert[jj] == meshtypes::snake)
                {
                    dvList.push_back(triRSVS.trisurf(subTemp)->indvert[jj]);
                }
            }
        }
    }
    dvListMap.vec = dvList;
    sort(dvListMap.vec);
    unique(dvListMap.vec);
    dvListMap.GenerateHash();
    nDvAct = dvListMap.vec.size();

    // Positional Derivatives

    // HERE -> function to calculate SurfCentroid (dc/dd)^T Hm (dc/dd)

    HPos.setZero(nDvAct, nDvAct);
    dPos.setZero(9, nDvAct);
    for (ii = 0; ii < ni; ++ii)
    {
        if (triIn.pointtype[ii] == meshtypes::snake && this->dvMap.find(triIn.pointind[ii]) != __notfound)
        {
            subTemp = triRSVS.snakeDep->snaxs.find(triIn.pointind[ii]);
            subTemp1 = triRSVS.meshDep->verts.find(triRSVS.snakeDep->snaxs(subTemp)->fromvert);
            subTemp2 = triRSVS.meshDep->verts.find(triRSVS.snakeDep->snaxs(subTemp)->tovert);
            subTemp3 = dvListMap.find(triIn.pointind[ii]);
            for (jj = 0; jj < 3; ++jj)
            {
                dPos(ii * 3 + jj, subTemp3) +=
                    triRSVS.meshDep->verts(subTemp2)->coord[jj] - triRSVS.meshDep->verts(subTemp1)->coord[jj];
            }
        }
        else if (triIn.pointtype[ii] == meshtypes::triangulation && false)
        {
        }
    }
    // Total

    if (false)
    {
        RSVS3D_ERROR_ARGUMENT("2D Area constraint not implemented yet");
        HVal.setZero(9, 9);
        dVal.setZero(1, 9);
        dConstrPart.setZero(1, nDvAct);
        HConstrPart.setZero(nDvAct, nDvAct);
        AreaCalc.Calc();
        AreaCalc.ReturnDatPoint(&retVal, &dValpnt, &HValpnt);
        ArrayVec2MatrixXd(*HValpnt, HVal);
        ArrayVec2MatrixXd(*dValpnt, dVal);
        Deriv1stChainScalar(dVal, dPos, dConstrPart);
        Deriv2ndChainScalar(dVal, dPos, HVal, HPos, HConstrPart);
    }

    if (isObj)
    {
        HVal.setZero(9, 9);
        dVal.setZero(1, 9);
        objPart = 0;
        for (int ed = 0; ed < 3; ++ed)
        {
            if (triIn.pointtype[ed] == meshtypes::snake && triIn.pointtype[(ed + 1) % 3] == meshtypes::snake)
            {
                HVal2.setZero(6, 6);
                dVal2.setZero(1, 6);
                EdgeCalc.assign(0, (triRSVS.snakeDep->snakeconn.verts.isearch(triIn.pointind[ed])->coord));
                EdgeCalc.assign(1, (triRSVS.snakeDep->snakeconn.verts.isearch(triIn.pointind[(ed + 1) % 3])->coord));
                EdgeCalc.Calc();
                EdgeCalc.ReturnDatPoint(&retVal, &dValpnt, &HValpnt);
                objPart += *retVal;
                ArrayVec2MatrixXd(*HValpnt, HVal2);
                ArrayVec2MatrixXd(*dValpnt, dVal2);
                for (ii = 0; ii < 6; ++ii)
                {
                    dVal(0, (ed * 3 + ii) % 9) += dVal2(0, ii);
                    for (jj = 0; jj < 6; ++jj)
                    {
                        HVal((ed * 3 + jj) % 9, (ed * 3 + ii) % 9) += HVal2(0, jj);
                    }
                }
            }
        }

        Deriv1stChainScalar(dVal, dPos, dObjPart);
        Deriv2ndChainScalar(dVal, dPos, HVal, HPos, HObjPart);
        // Assign to main part of the object
        // assign objective function
        this->obj += objPart;
        // Assign objective derivative
        if (isDeriv)
        {
            for (ii = 0; ii < nDvAct; ++ii)
            {
                this->dObj[this->dvMap.find(dvListMap.vec[ii])] += dObjPart(0, ii);
                for (jj = 0; jj < nDvAct; ++jj)
                {
                    this->HObj(dvMap.find(dvListMap.vec[jj]), this->dvMap.find(dvListMap.vec[ii])) += HObjPart(jj, ii);
                }
            }
        }
    }

    // Update active lists of design variables
    for (ii = 0; ii < nDvAct; ++ii)
    {
        this->isDvAct.at(dvMap.find(dvListMap.vec[ii])) = true;
    }
    nCellTarg = triIn.connec.celltarg.size();
    for (ii = 0; ii < nCellTarg; ++ii)
    {
        subTempVec = constrMap.findall(triIn.connec.celltarg[ii]);
        nj = subTempVec.size();
        for (jj = 0; jj < nj; ++jj)
        {
            if (subTempVec[jj] != __notfound)
            {
                this->isConstrAct.at(subTempVec[jj]) = true;
            }
        }
    }

    // Assign Objective Hessian

    // Assign Constraint Hessian
}
