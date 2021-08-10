
#include "RSVScalc.hpp"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "matrixtools.hpp"
#include "snake.hpp"
#include "triangulate.hpp"
#include "vectorarray.hpp"

/**
 * Tests if data is accessible then scales it if it can.
 *
 * @param      val   The value to access
 * @param      ind   The index to access the data is not accessed if the indes
 *                   is equal to rsvs3d::constants::__notfound.
 *
 * @return     the scaled log scale (see rsvs3d::SignedLogScale)
 */
#define TOSENSVEC(val, ind) rsvs3d::SignedLogScale((ind != rsvs3d::constants::__notfound ? val : 0.0))

using namespace std;

// silent functions

/*
Implementation of the interfaces for RSVScalc
This file's object can be compiled quickly
*/

void RSVScalc::PrepTriangulationCalc(const triangulation &triRSVS)
{
    int ii;
    int nDv, nConstr;
    vector<int> vecin;
    // prepare the SQP object
    nConstr = triRSVS.meshDep->CountVoluParent();
    if (triRSVS.snakeDep != NULL)
    {
        nDv = triRSVS.snakeDep->snaxs.size();
    }
    else
    {
        nDv = 0;
    }

    // TODO this needs to be supported by mapping each volume to the constraint
    // position There can be more than one constraint for each cell.
    BuildConstrMap(triRSVS);

    vecin.clear();
    vecin.reserve(nDv);
    for (ii = 0; ii < nDv; ++ii)
    {
        if (!triRSVS.snakeDep->snaxs(ii)->isfreeze)
        {
            vecin.push_back(triRSVS.snakeDep->snaxs(ii)->index);
        }
        else if (this->SnakDVcond(triRSVS, ii))
        {
            vecin.push_back(triRSVS.snakeDep->snaxs(ii)->index);
        }
    }
    nDv = this->BuildDVMap(vecin);

    BuildMathArrays(nDv, nConstr);
}

bool RSVScalc::SnakDVcond(const triangulation &triRSVS, int ii)
{
    bool allNeighFroze = true;
    int nEdges, kk;
    const edge *tempEdge;

    nEdges = triRSVS.snakeDep->snakeconn.verts(ii)->edgeind.size();

    kk = 0;
    // Check if all neighbours of the snaxel are
    while (kk < nEdges && allNeighFroze)
    {
        tempEdge = triRSVS.snakeDep->snakeconn.edges.isearch(triRSVS.snakeDep->snakeconn.verts(ii)->edgeind[kk]);
        allNeighFroze &= triRSVS.snakeDep->snaxs.isearch(tempEdge->vertind[0])->isfreeze;
        allNeighFroze &= triRSVS.snakeDep->snaxs.isearch(tempEdge->vertind[1])->isfreeze;

        ++kk;
    }

    return (!allNeighFroze);
}

void RSVScalc::CalculateTriangulation(const triangulation &triRSVS, int derivMethod)
{
    int ii, ni;

    this->returnDeriv = true;

    // prepare the SQP object
    this->PrepTriangulationCalc(triRSVS);

    // Calculate the SQP object
    this->ZeroTimers();
    auto calcTriFunc = &RSVScalc::CalcTriangle;
    switch (derivMethod)
    {
    case 1:
        calcTriFunc = &RSVScalc::CalcTriangleFD;
        break;
    case 2:
        calcTriFunc = &RSVScalc::CalcTriangleDirectVolume;
        break;
    }
    this->ZeroTimers();
    ni = triRSVS.dynatri.size();
    for (ii = 0; ii < ni; ii++)
    {
        (this->*calcTriFunc)(*(triRSVS.dynatri(ii)), triRSVS, true, true, true);
    }

    ni = triRSVS.intertri.size();
    for (ii = 0; ii < ni; ii++)
    {
        (this->*calcTriFunc)(*(triRSVS.intertri(ii)), triRSVS, false, true, true);
    }

    ni = triRSVS.acttri.size();
    for (ii = 0; ii < ni; ii++)
    {
        (this->*calcTriFunc)(*(triRSVS.stattri.isearch(triRSVS.acttri[ii])), triRSVS, false, true, false);
    }

    // Output some data to check it makes sense
}

/**
 * @brief      Returns velocities to the snake in the triangulation object.
 *
 * @param      triRSVS  The triangulation object of the RSVS
 */
void RSVScalc::ReturnVelocities(triangulation &triRSVS)
{
    int ii, ni, temp;

    ni = triRSVS.snakeDep->snaxs.size();
    for (ii = 0; ii < ni; ii++)
    {
        temp = this->dvMap.find(triRSVS.snakeDep->snaxs(ii)->index);
        triRSVS.snakeDep->snaxs[ii].v = temp != -1 ? this->deltaDV[temp] : 0;
    }
    triRSVS.snakeDep->snaxs.PrepareForUse();
}

void RSVScalc::ReturnSensitivities(const triangulation &triRSVS, std::vector<double> &sensVec, int constrNum) const
{
    int ii, ni, temp;

    if (constrNum >= this->nConstr)
    {
        RSVS3D_ERROR_RANGE("Constraint is beyond the available range.");
    }
    ni = triRSVS.snakeDep->snaxs.size();
    sensVec.assign(ni, 0);
    for (ii = 0; ii < ni; ii++)
    {
        temp = this->dvMap.find(triRSVS.snakeDep->snaxs(ii)->index);
        sensVec[ii] = TOSENSVEC(this->sensDv(temp, constrNum), temp);
    }
}

void RSVScalc::ReturnGradient(const triangulation &triRSVS, std::vector<double> &sensVec, int constrNum) const
{
    int ii, ni, temp;
    int nNegOpts = 6;

    // auto TOSENSVEC = [&](double val, int ind) -> double
    // 	{return rsvs3d::SignedLogScale(
    // 		(ind!=rsvs3d::constants::__notfound?val:0.0));};

    if (constrNum >= this->nConstr || constrNum < -nNegOpts)
    {
        RSVS3D_ERROR_RANGE("Constraint is beyond the available range.");
    }
    ni = triRSVS.snakeDep->snaxs.size();
    sensVec.assign(ni, 0);
    int nNegTest = -nNegOpts;
    if (constrNum == nNegTest++)
    {
        for (ii = 0; ii < ni; ii++)
        {
            temp = this->dvMap.find(triRSVS.snakeDep->snaxs(ii)->index);
            sensVec[ii] = TOSENSVEC(this->dObj[temp], temp);
        }
    }
    else if (constrNum == nNegTest++)
    {
        auto dLagTemp = this->dObj + this->lagMult.transpose() * this->dConstr;
        for (ii = 0; ii < ni; ii++)
        {
            temp = this->dvMap.find(triRSVS.snakeDep->snaxs(ii)->index);
            sensVec[ii] = TOSENSVEC(dLagTemp[temp], temp);
        }
    }
    else if (constrNum == nNegTest++)
    {
        if (this->UseFullMath())
        {
            for (ii = 0; ii < ni; ii++)
            {
                temp = this->dvMap.find(triRSVS.snakeDep->snaxs(ii)->index);
                sensVec[ii] = TOSENSVEC(this->HObj(temp, temp), temp);
            }
        }
        else
        {
            for (ii = 0; ii < ni; ii++)
            {
                temp = this->dvMap.find(triRSVS.snakeDep->snaxs(ii)->index);
                sensVec[ii] = TOSENSVEC(this->HObj_sparse(temp, temp), temp);
            }
        }
    }
    else if (constrNum == nNegTest++)
    {
        if (this->UseFullMath())
        {
            for (ii = 0; ii < ni; ii++)
            {
                temp = this->dvMap.find(triRSVS.snakeDep->snaxs(ii)->index);
                sensVec[ii] = TOSENSVEC(this->HConstr(temp, temp), temp);
            }
        }
        else
        {
            for (ii = 0; ii < ni; ii++)
            {
                temp = this->dvMap.find(triRSVS.snakeDep->snaxs(ii)->index);
                sensVec[ii] = TOSENSVEC(this->HConstr_sparse(temp, temp), temp);
            }
        }
    }
    else if (constrNum == nNegTest++)
    {
        if (this->UseFullMath())
        {
            auto HLagTemp = this->HConstr + this->HObj;
            for (ii = 0; ii < ni; ii++)
            {
                temp = this->dvMap.find(triRSVS.snakeDep->snaxs(ii)->index);
                sensVec[ii] = TOSENSVEC(HLagTemp(temp, temp), temp);
            }
        }
        else
        {
            for (ii = 0; ii < ni; ii++)
            {
                temp = this->dvMap.find(triRSVS.snakeDep->snaxs(ii)->index);
                sensVec[ii] = TOSENSVEC((this->HConstr_sparse(temp, temp) + this->HObj_sparse(temp, temp)), temp);
            }
        }
    }
    else if (constrNum == nNegTest++)
    {
        if (this->UseFullMath())
        {
            for (ii = 0; ii < ni; ii++)
            {
                temp = this->dvMap.find(triRSVS.snakeDep->snaxs(ii)->index);
                sensVec[ii] = TOSENSVEC(this->deltaDV[temp], temp);
            }
        }
        else
        {
        }
    }
    else
    {
        if (constrNum < 0)
        {
            RSVS3D_ERROR_LOGIC("Negative ''constraints'' should be handled in"
                               " before this case.");
        }
        for (ii = 0; ii < ni; ii++)
        {
            temp = this->dvMap.find(triRSVS.snakeDep->snaxs(ii)->index);
            sensVec[ii] = TOSENSVEC(this->dConstr(constrNum, temp), temp);
        }
    }
}

void RSVScalc::CalculateMesh(mesh &meshin)
{
    int ii, ni, jj, nj;
    int nDv, nConstr;
    // vector<int> vecin;
    triangulation triRSVS(meshin);
    // prepare the SQP object
    triRSVS.PrepareForUse();
    this->returnDeriv = false;
    nConstr = meshin.volus.size();

    nDv = 0;

    BuildMathArrays(nDv, nConstr);

    // TODO this needs to be supported by mapping each volume to the constraint
    // position There can be more than one constraint for each cell.
    // BuildConstrMap(triRSVS);
    BuildConstrMap(meshin);

    // Calculate the SQP object
    nj = meshin.surfs.size();
    for (jj = 0; jj < nj; jj++)
    {
        TriangulateSurface(*(meshin.surfs(jj)), meshin, triRSVS.stattri, triRSVS.trivert, 1, 1);
        triRSVS.PrepareForUse();
        triRSVS.CalcTriVertPos();
        triRSVS.PrepareForUse();
        ni = triRSVS.stattri.size();
        for (ii = 0; ii < ni; ii++)
        {
            CalcTriangle(*(triRSVS.stattri(ii)), triRSVS, true, true, false);
        }
        triRSVS.stattri.clear();
        triRSVS.trivert.clear();
    }

    // Output some data to check it makes sense
}

void RSVScalc::Print2Screen(int outType) const
{
    cout << "Math result: obj " << obj << " false access " << falseaccess << endl;
    cout << "Constr ";
    for (int i = 0; i < nConstr; ++i)
    {
        cout << constr[i] << " ";
    }
    cout << "constrTarg :" << endl;
    PrintMatrix(constrTarg);
    cout << endl;
    if ((nConstr < 10 && nDv < 20 && outType == 1) || outType == 3)
    {
        cout << "constr :" << endl;
        PrintMatrix(constr);
        cout << "dObj :" << endl;
        PrintMatrix(dObj);
        cout << "lagMult :" << endl;
        PrintMatrix(lagMult);

        cout << "dConstr :" << endl;
        PrintMatrix(dConstr);
        cout << "HConstr :" << endl;
        PrintMatrix(HConstr);
        cout << "HObj :" << endl;
        PrintMatrix(HObj);
        cout << "constrTarg :" << endl;
        PrintMatrix(constrTarg);
    }
    if (outType == 2)
    {
        cout << "constrTarg :" << endl;
        PrintMatrix(constrTarg);
        cout << endl;
        for (int i = 0; i < nDv; ++i)
        {
            cout << deltaDV[i] << " ";
        }
        cout << endl;
        for (int i = 0; i < nConstr; ++i)
        {
            cout << lagMult[i] << " ";
        }
        cout << endl;
    }
    if (outType == 4)
    {
        const char *file = "matrices/dumpmatout.txt";
        cout << "constr :" << endl;
        PrintMatrixFile(constr, file);
        cout << "dObj :" << endl;
        PrintMatrixFile(dObj, file);
        cout << "lagMult :" << endl;
        PrintMatrixFile(lagMult, file);

        cout << "dConstr :" << endl;
        PrintMatrixFile(dConstr, file);
        cout << "HConstr :" << endl;
        PrintMatrixFile(HConstr, file);
        cout << "HObj :" << endl;
        PrintMatrixFile(HObj, file);
        cout << "constrTarg :" << endl;
        PrintMatrixFile(constrTarg, file);
    }
}

void RSVScalc::ReturnConstrToMesh(triangulation &triRSVS) const
{
    int ii, ni;
    vector<double> temp;
    ni = constr.size();
    double tempVal;
    temp.reserve(ni);

    for (ii = 0; ii < ni; ii++)
    {
        temp.push_back(constr[ii]);
    }
    triRSVS.meshDep->MapVolu2Parent(temp, this->constrList, &volu::fill);
    temp.clear();
    for (ii = 0; ii < ni; ii++)
    {
        tempVal = fabs(constrTarg[ii] - constr[ii]);
        if (tempVal > 1e-16)
        {
            temp.push_back(log10(tempVal));
        }
        else
        {
            temp.push_back(-16);
        }
    }
    triRSVS.meshDep->MapVolu2Parent(temp, this->constrList, &volu::error);
}

void RSVScalc::ReturnConstrToMesh(mesh &meshin, double volu::*mp) const
{
    int ii, ni;
    vector<double> temp;
    ni = constr.size();
    temp.reserve(ni);

    for (ii = 0; ii < ni; ii++)
    {
        temp.push_back(constr[ii]);
    }
    meshin.MapVolu2Self(temp, constrMap.vec, mp);
}

void RSVScalc::BuildMathArrays(int nDvIn, int nConstrIn)
{
    // Builds the target math arrays

    this->nDv = nDvIn;
    this->nConstr = nConstrIn;
    this->isConstrAct.clear();
    this->isDvAct.clear();
    this->isConstrAct.assign(nConstr, false);
    this->isDvAct.assign(nDv, false);

    this->constr.setZero(nConstr);

    this->obj = 0.0;
    this->dObj.setZero(nDv);

    if (this->nConstr != this->lagMult.size())
    {
        this->lagMult.setZero(nConstr);
    }
    this->deltaDV.setZero(nDv);

    this->dvCallConstr.setZero(nDv, 1);

    if (this->UseFullMath())
    {
        this->dConstr.setZero(nConstr, nDv);
        this->HConstr.setZero(nDv, nDv);
        this->HObj.setZero(nDv, nDv);
    }
    else
    {
        this->dConstr_sparse.setZero();
        this->HConstr_sparse.setZero();
        this->HObj_sparse.setZero();
        this->HLag_sparse.setZero();
        this->dConstr_sparse.resize(nConstr, nDv);
        this->dConstr_sparse.reserve(nDv * 2);
        this->HConstr_sparse.resize(nDv, nDv);
        this->HConstr_sparse.reserve(nDv * 13);
        this->HObj_sparse.resize(nDv, nDv);
        this->HObj_sparse.reserve(nDv * 13);
    }
}

void RSVScalc::BuildConstrMap(const triangulation &triangleRSVS)
{
    // explore parents of mesh adding 1 by 1 elemind
    // for each parent
    // for each snakemesh().volu
    // Assign to constrMap.targ = the position in parent.volu of the parentconn
    vector<double> voluVals;
    triangleRSVS.meshDep->ReturnParentMap(constrMap.vec, constrMap.targ, constrList, voluVals);

    constrMap.GenerateHash();
    constrTarg.setZero(voluVals.size());
    for (int i = 0; i < int(voluVals.size()); ++i)
    {
        constrTarg[i] = voluVals[i];
    }
}

void RSVScalc::BuildConstrMap(const mesh &meshin)
{
    int ni, ii;
    ni = meshin.volus.size();
    constrMap.vec.clear();
    constrMap.targ.clear();
    constrMap.vec.reserve(ni);
    constrMap.targ.reserve(ni);
    for (ii = 0; ii < ni; ++ii)
    {
        constrMap.vec.push_back(meshin.volus(ii)->index);
        constrMap.targ.push_back(ii);
    }

    constrMap.GenerateHash();
}

int RSVScalc::BuildDVMap(const vector<int> &vecin)
{
    this->dvMap.vec = vecin;
    sort(this->dvMap.vec);
    unique(this->dvMap.vec);
    this->dvMap.GenerateHash();
    return this->dvMap.vec.size();
}

void RSVScalc::ConvergenceLog(ofstream &out, int loglvl) const
{
    /*
    takes in a stream and a log lvl (which defaults to the highest available)
    */
    // format stream  (in preparation)

    // residual and delta are summary with:
    // mean median max min std
    double normConstr, normVel, normObjDeriv;
    if (loglvl > 0)
    {
        out << "> constraint residual :, ";
        normConstr = StreamStatistics(abs(this->constr.array() - this->constrTarg.array()), out, string(", "));
        out << "> constraint delta :, ";
        StreamStatistics((this->constr.array() - this->constrTarg.array()), out, string(", "));
        out << "> objective residual :, ";
        StreamStatistics(abs(this->deltaDV.array()), out, string(", "));
        out << "> objective delta :, ";
        normVel = StreamStatistics(this->deltaDV.array(), out, string(", "));
        out << "> objective derivative :, ";
        normObjDeriv = StreamStatistics(this->dObj.array(), out, string(", "));
        out << "> objective value:," << this->obj << std::endl;
        auto prec = std::cout.precision();
        std::cout << " conv:" << std::setprecision(3) << std::left << " (vol) " << std::setw(8) << normConstr
                  << " (vel) " << std::setw(8) << normVel << " (Dobj) " << std::setw(8) << normObjDeriv << "; "
                  << std::setprecision(prec) << std::right;
    }
    // Same as res but with all the constraint values
    if (loglvl > 1)
    {
        out << "> constraint residuals :, ";
        StreamOutVector(abs(this->constr.array() - this->constrTarg.array()), out, string(", "));

        out << "> volume values :, ";
        StreamOutVector(this->constr.array(), out, string(", "));

        out << "> constraint deltas :, ";
        StreamOutVector((this->constr.array() - this->constrTarg.array()), out, string(", "));
    }
    if (loglvl > 2)
    {
        out << "> snaxel velocity :, ";
        StreamOutVector(move(deltaDV), out, string(", "));
    }
}

void RSVScalc::PrintTimers() const
{
    std::cout << " t1 " << rsvs3d::Clock2ms(this->timer1) << ",";
    std::cout << " t2 " << rsvs3d::Clock2ms(this->timer2) << ",";
    std::cout << " t3 " << rsvs3d::Clock2ms(this->timer3) << ",";
}

void SparseMatrixTriplet::SetEqual(MatrixXd_sparse &targ)
{
    int n = this->nonZeros();
    targ.setZero();
    targ.resize(this->nRow, this->nCol);
    targ.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        auto it = this->operator[](i);
        targ.insert(it.row(), it.col()) = it.value();
    }
}

void SparseMatrixTriplet::SetEqual(Eigen::MatrixXd &targ)
{
    int n = this->nonZeros();
    targ.resize(this->nRow, this->nCol);
    targ.setZero(this->nRow, this->nCol);
    for (int i = 0; i < n; ++i)
    {
        auto it = this->operator[](i);
        targ(it.row(), it.col()) = it.value();
    }
}

void SparseMatrixTriplet::reserve(size_t a)
{
    this->TripletMap::reserve(a);
}

sparsetripletelement<double> SparseMatrixTriplet::operator[](int a)
{
#ifdef SAFE_ACCESS
    if (a < 0 || a >= int(this->TripletMap::size()))
    {
        RSVS3D_ERROR_RANGE("Indices out of range.");
    }
#endif
    sparsetripletelement<double> out(this->vec[a] % this->nRow, this->vec[a] / this->nRow, this->targ[a]);
    return out;
}
