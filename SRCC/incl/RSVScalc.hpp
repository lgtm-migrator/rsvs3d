/**
 * Provides the infrastructure for calculation of the RSVS equations.
 *
 *@file
 */

#ifndef RSVSCALC_H_INCLUDED
#define RSVSCALC_H_INCLUDED

//=================================
// forward declared dependencies
// 		class foo; //when you only need a pointer not the actual object
// 		and to avoid circular dependencies

//=================================
// included dependencies
#include <Eigen>
#include <fstream>
#include <iostream>
#include <vector>

#include "arraystructures.hpp"
#include "mesh.hpp"
#include "snake.hpp"
#include "triangulate.hpp"
#include "warning.hpp"

typedef Eigen::SparseMatrix<double, Eigen::ColMajor> MatrixXd_sparse;
typedef HashedVectorPair<int, int, int, double> TripletMap;

template <typename T> class sparsetripletelement
{
    int rowVal = 0;
    int colVal = 0;
    T valueVal = 0;

  public:
    sparsetripletelement<T>(int r, int c, T v)
    {
#ifdef SAFE_ACCESS
        if (r < 0 || c < 0)
        {
            RSVS3D_ERROR_RANGE("Indices out of range.");
        }
#endif
        this->rowVal = r;
        this->colVal = c;
        this->valueVal = v;
    }
    int row() const
    {
        return this->rowVal;
    }
    int col() const
    {
        return this->colVal;
    }
    T value() const
    {
        return this->valueVal;
    }
};

class SparseMatrixTriplet : protected TripletMap
{
    int nRow = 0;
    int nCol = 0;

  public:
    double &operator()(int a, int b)
    {
#ifdef SAFE_ACCESS
        if (a < 0 || a >= this->nRow || b < 0 || b >= this->nCol)
        {
            RSVS3D_ERROR_RANGE("Indices out of range.");
        }
#endif
        return TripletMap::operator()(a + this->nRow * b);
    }
    const double &operator()(int a, int b) const
    {
#ifdef SAFE_ACCESS
        if (a < 0 || a >= this->nRow || b < 0 || b >= this->nCol)
        {
            RSVS3D_ERROR_RANGE("Indices out of range.");
        }
#endif
        return TripletMap::operator()(a + this->nRow * b);
    }
    sparsetripletelement<double> operator[](int a);
    double &coeffRef(int a, int b)
    {
        return this->operator()(a, b);
    }
    void resize(int row, int col)
    {
        this->nRow = row;
        this->nCol = col;
    }
    void setZero()
    {
        this->TripletMap::clear();
    }
    int nonZeros() const
    {
        return this->TripletMap::size();
    }
    void reserve(size_t a);
    void SetEqual(MatrixXd_sparse &targ);
    void SetEqual(Eigen::MatrixXd &targ);
};

/**
 * Class to handle the RSVS calculation.
 *
 * This class calculates volume and area metrics in a triangulated snake
 * to update the velocity and volumes. It uses an SQP algorithm to compute the
 * velocities.
 */
class RSVScalc
{
  protected:
    /// Number of design variables
    int nDv = 0;
    /// Number of constraints
    int nConstr = 0;
    /// Number of false access operations
    int falseaccess = 0;
    /// Number of design variables to start using sparse mathematics
    int sparseDVcutoff = 200;
    ///
    int nonZeroPerDV = 10;
    /// Return the derivatives (obsolete/unused)
    bool returnDeriv = true;
    /// Enable or disable surfcentroid derivatives
    bool useSurfCentreDeriv = true;
    /// Enable or disable surfcentroid derivatives
    bool useSurfCentreHessian = false;

    int timer1 = 0;
    int timer2 = 0;
    int timer3 = 0;
    void ZeroTimers()
    {
        this->timer1 = 0;
        this->timer2 = 0;
        this->timer3 = 0;
    }
    void PrintTimers() const;

  public:
    /// Constraint Jacobian, size: [nConstr, nDv].
    Eigen::MatrixXd dConstr;
    SparseMatrixTriplet dConstr_sparse;
    /// Constraint Hessian, size: [nDv, nDv].
    Eigen::MatrixXd HConstr;
    SparseMatrixTriplet HConstr_sparse;
    /// Objective Hessian, size: [nDv, nDv].
    Eigen::MatrixXd HObj;
    SparseMatrixTriplet HObj_sparse;
    /// Lagrangian Hessian, size: [nDv, nDv].
    Eigen::MatrixXd HLag;
    MatrixXd_sparse HLag_sparse;
    Eigen::RowVectorXd dLag;
    /// Objective Jacobian, size: [1, nDv].
    Eigen::RowVectorXd dObj;
    /// Constraint value vector, size: [nConstr, 1].
    Eigen::VectorXd constr;
    /// Lagrangian multiplier, size: [nConstr, 1].
    Eigen::VectorXd lagMult;
    /// Change in design variable, assigned to snake velocity, size: [nDv, 1].
    Eigen::VectorXd deltaDV;
    /// Constraint target values, size: [nConstr, 1].
    Eigen::VectorXd constrTarg;
    ///
    Eigen::MatrixXd dvCallConstr;
    /// Sensitivity of the optimum design variables to the constraint.
    Eigen::MatrixXd sensDv;
    /// Objective function value.
    double obj = 0.0;
    /// Value at which a Lagrangian multiplier is considered problematically
    /// large
    double limLag = INFINITY;
    /// is the corresponding constraint active?
    std::vector<bool> isConstrAct;
    /// Is the corresponding design variable active?
    std::vector<bool> isDvAct;
    /// Vector of subscripts of the active constraints
    HashedVector<int, int> subConstrAct;
    /// Vector of subscripts of the active design variables
    HashedVector<int, int> subDvAct;
    /// Maps the snake indices to the position in the design variable vector
    HashedVector<int, int> dvMap;
    /// maps snakemesh() volu onto constr
    HashedMap<int, int, int> constrMap;
    /// keeps pairs with parentindex and voluindex
    std::vector<std::pair<int, int>> constrList;

    /**
     * @brief      Decides wether full or sparse math should be used.
     *
     * @return     True if the number of design variable is small enought to
     *             allow full matrix operations and false if the sparse maths
     *             should be used.
     */
    bool UseFullMath() const
    {
        return (this->nDv < this->sparseDVcutoff);
    }
    /**
     * @brief      Builds mathematics arrays.
     *
     * @param[in]  nDv      Number of design variables.
     * @param[in]  nConstr  Number of constraints.
     */
    void BuildMathArrays(int nDv, int nConstr);

    /**
     * @brief      Builds the constraint mapping.
     *
     * @param[in]  triangleRSVS  Triangulation containing the RSVS.
     */
    void BuildConstrMap(const triangulation &triangleRSVS);

    /**
     * @brief      Builds the constraint mapping.
     *
     * @param[in]  meshin  mesh for constraint building.
     */
    void BuildConstrMap(const mesh &meshin);

    /**
     * @brief      Builds a Design variable map.
     *
     * @param[in]  vecin  The input vector of design variable indices.
     *
     * @return     The number of design variable.
     */
    int BuildDVMap(const std::vector<int> &vecin);

    /**
     * Returns wether a snaxel is a design variable or not.
     *
     * If the snaxel is frozen and all its neighbours are frozen, it is not
     * a design variable.
     *
     * @param[in]  triRSVS  The triangulation which is being calculated
     * @param[in]  ii       the snaxel subscript.
     *
     * @return     wether the snaxel is design variable or not.
     */
    bool SnakDVcond(const triangulation &triRSVS, int ii);

    /**
     * @brief      Groups actions needed before the calculation of triangular
     * quantities.
     *
     * @param[in]  triRSVS  The triangulation object.
     */
    void PrepTriangulationCalc(const triangulation &triRSVS);

    /**
     * @brief      Calculates the mesh volumes.
     *
     * @param      meshin  The mesh.
     */
    void CalculateMesh(mesh &meshin);

    /**
     * @brief      Calculates the triangulation volume and area derivatives.
     *
     * @param[in]  triRSVS      The triangle rsvs
     * @param[in]  derivMethod  The differentiation method to use. 1 : Finite
     *                          Difference, 2 : Direct calculation, all others :
     *                          differentiation.
     */
    void CalculateTriangulation(const triangulation &triRSVS, int derivMethod = 0);

    /**
     * Calculates the properties of single triangle.
     *
     * These values are returned to the class math arrays.
     *
     * @param[in]  triIn     The triangle to measure.
     * @param[in]  triRSVS   The containing triangulation object.
     * @param[in]  isObj     Calculate objective?
     * @param[in]  isConstr  Calculate constraint?
     * @param[in]  isDeriv   Calculate derivatives?
     */
    void CalcTriangle(const triangle &triIn, const triangulation &triRSVS, bool isObj = true, bool isConstr = true,
                      bool isDeriv = true);

    /**
     * Calculates the properties of single triangle using Finite difference.
     *
     * These values are returned to the class math arrays.
     *
     * @param[in]  triIn     The triangle to measure.
     * @param[in]  triRSVS   The containing triangulation object.
     * @param[in]  isObj     Calculate objective?
     * @param[in]  isConstr  Calculate constraint?
     * @param[in]  isDeriv   Calculate derivatives?
     */
    void CalcTriangleFD(const triangle &triIn, const triangulation &triRSVS, bool isObj = true, bool isConstr = true,
                        bool isDeriv = true);

    /**
     * Calculates the properties of single triangle using direct calculation.
     *
     * These values are returned to the class math arrays.
     *
     * @param[in]  triIn     The triangle to measure.
     * @param[in]  triRSVS   The containing triangulation object.
     * @param[in]  isObj     Calculate objective?
     * @param[in]  isConstr  Calculate constraint?
     * @param[in]  isDeriv   Calculate derivatives?
     */
    void CalcTriangleDirectVolume(const triangle &triIn, const triangulation &triRSVS, bool isObj = true,
                                  bool isConstr = true, bool isDeriv = true);

    /**
     * Calculates the properties of single triangle for 2D RSVS.
     *
     * These values are returned to the class math arrays.
     *
     * @param[in]  triIn     The triangle to measure.
     * @param[in]  triRSVS   The containing triangulation object.
     * @param[in]  isObj     Calculate objective?
     * @param[in]  isConstr  Calculate constraint?
     * @param[in]  isDeriv   Calculate derivatives?
     */
    void CalcTriangleEdgeLength(const triangle &triIn, const triangulation &triRSVS, bool isObj = true,
                                bool isConstr = true, bool isDeriv = true);

    /**
     * @brief      Returns a constraint to the triangulation::meshDep.
     *
     * @param      triRSVS  The triangulation object.
     */
    void ReturnConstrToMesh(triangulation &triRSVS) const;

    /**
     * @brief      Returns a constraint to the mesh.
     *
     * @param      meshin  The input mesh.
     * @param[in]  volu    The volumetric field that data needs to be returned
     *                     to. It is a member point of class volu.
     */
    void ReturnConstrToMesh(mesh &meshin, double volu::*mp = &volu::volume) const;

    /**
     * @brief      Prepare the active arrays for SQP calculation and calculate
     *             the SQP step.
     *
     * @param[in]  calcMethod  Calculation method for SQP. Check
     *                         :meth:RSVScalc::ComputeSQPstep for detail.
     */
    void CheckAndCompute(int calcMethod = 0, bool sensCalc = false);

    /**
     * Calculates the next SQP step.
     *
     * In normal operation the constraint should be 0 through 4. With 0 the
     * default. By adding 10 to these values the "constraint only" mode is
     * enabled which performs a gradient descent step based on the constraint.
     *
     * @param[in]  calcMethod  The calculation method. 10 can be added to all
     *                         values to enable the "constraint only" mode.
     *                         Values correspond to the following:
     *                         `Eigen::HouseholderQR` (1); 	 *
     *                         `Eigen::ColPivHouseholderQR` (2) - Default;
     *                         Eigen::LLT<Eigen::MatrixXd> (3);
     * `Eigen::PartialPivLU` (4);
     * @param      dConstrAct  The active constraint Jacobian
     * @param      dObjAct     The active objective Jacobian
     * @param      constrAct   The active constraint values
     * @param      lagMultAct  The active lagrangian multipliers.
     */
    void ComputeSQPstep(int calcMethod, Eigen::MatrixXd &dConstrAct, Eigen::RowVectorXd &dObjAct,
                        Eigen::VectorXd &constrAct, Eigen::VectorXd &lagMultAct);
    void ComputeSQPstep(int calcMethod, MatrixXd_sparse &dConstrAct, Eigen::RowVectorXd &dObjAct,
                        Eigen::VectorXd &constrAct, Eigen::VectorXd &lagMultAct);

    void ComputeSQPsens(int calcMethod, const Eigen::MatrixXd &sensMult, const Eigen::MatrixXd &sensInv,
                        Eigen::MatrixXd &sensRes);
    void ComputeSQPsens(int calcMethod, Eigen::MatrixXd &sensMult, MatrixXd_sparse &sensInv, Eigen::MatrixXd &sensRes);

    /**
     * @brief      Prepares the matrices needed for the SQP step calculation.
     *
     * @param      dConstrAct  The active constraint Jacobian
     * @param      HConstrAct  The active constraint hessian
     * @param      HObjAct     The active objective hessian
     * @param      dObjAct     The active objective Jacobian
     * @param      constrAct   The active constraint values
     * @param      lagMultAct  The active lagrangian multipliers.
     *
     * @return     Returns wether the calculation should be performed or not.
     */
    bool PrepareMatricesForSQP(Eigen::MatrixXd &dConstrAct, Eigen::MatrixXd &HConstrAct, Eigen::MatrixXd &HObjAct,
                               MatrixXd_sparse &dConstrAct_sparse, MatrixXd_sparse &HConstrAct_sparse,
                               MatrixXd_sparse &HObjAct_sparse, Eigen::RowVectorXd &dObjAct, Eigen::VectorXd &constrAct,
                               Eigen::VectorXd &lagMultAct);

    void PrepareMatricesForSQPFull(Eigen::MatrixXd &dConstrAct, Eigen::MatrixXd &HConstrAct, Eigen::MatrixXd &HObjAct);
    void PrepareMatricesForSQPSparse(MatrixXd_sparse &dConstrAct_sparse, MatrixXd_sparse &HConstrAct_sparse,
                                     MatrixXd_sparse &HObjAct_sparse);
    /**
     * @brief      Prepares the matrices needed for the calculation of
     * the sensitivity of the SQP.
     *
     * This is done to then call RSVScalc::ComputeSQPsens which implements
     * the SQP optimality sensitivity giving the change in design variables
     * due to a small change of constraint at the optimal condition.
     *
     * @param      dConstrAct  The active constraint Jacobian
     * @param      HConstrAct  The active constraint hessian
     * @param      HObjAct     The active objective hessian
     * @param      sensMult    The sensitivity RHS multiplier
     * @param      sensInv     The sensitivity RHS Matrix equation
     * @param      sensRes     The sensitivity LHS result matrix.
     *
     * @return     Returns if the sensitivity should be computed or not.
     */
    bool PrepareMatricesForSQPSensitivity(const Eigen::MatrixXd &dConstrAct, const Eigen::MatrixXd &HConstrAct,
                                          const Eigen::MatrixXd &HObjAct, Eigen::MatrixXd &sensMult,
                                          Eigen::MatrixXd &sensInv, Eigen::MatrixXd &sensRes) const;
    bool PrepareMatricesForSQPSensitivity(const MatrixXd_sparse &dConstrAct, const MatrixXd_sparse &HConstrAct,
                                          MatrixXd_sparse &HObjAct, Eigen::MatrixXd &sensMult, MatrixXd_sparse &sensInv,
                                          Eigen::MatrixXd &sensRes) const;

    /**
     * @brief      Returns velocities to the snaxels.
     *
     * @param      triRSVS  The triangulation object, affects the
     *                      triangulation::snakeDep attribute.
     */
    void ReturnVelocities(triangulation &triRSVS);
    void ReturnSensitivities(const triangulation &triRSVS, std::vector<double> &sensVec, int constrNum) const;
    void ReturnGradient(const triangulation &triRSVS, std::vector<double> &sensVec, int constrNum) const;
    /**
     * @brief      Getter for the number of constraints.
     *
     * @return     The number of constraints.
     */
    int numConstr() const
    {
        return (this->nConstr);
    }
    // Output functions

    /**
     * @brief      Prints different amounts of `RSVScalc` owned data to the
     *             screen.
     *
     * @param[in]  outType  The output type to print, values [2,3,4].
     */
    void Print2Screen(int outType = 0) const;

    /**
     * @brief      Print convergence information to file stream.
     *
     * @param      out     The output filestream
     * @param[in]  loglvl  The logging detail to output. <1 nothing, ==1 Vector
     *                     statistics, ==2 ...and constraint vectors, >2 ...and
     *                     snaxel velocity vector.
     */
    void ConvergenceLog(std::ofstream &out, int loglvl = 3) const;

    void SetUseSurfCentreDeriv(int in)
    {
        this->useSurfCentreDeriv = in;
    }
    bool GetUseSurfCentreDeriv() const
    {
        return (this->useSurfCentreDeriv);
    }
    void SetUseSurfCentreHessian(int in)
    {
        this->useSurfCentreHessian = in;
    }
    bool SetUseSurfCentreHessian() const
    {
        return (this->useSurfCentreHessian);
    }
    void SetSparseDVcutoff(int in)
    {
        this->sparseDVcutoff = in;
    }
    int GetSparseDVcutoff() const
    {
        return (this->sparseDVcutoff);
    }
};

#endif
