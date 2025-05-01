#pragma once
#include <iostream>
#include <format>
//#define  EIGEN_DONT_PARALLELIZE
#include "vendor/Eigen/Dense"
#include "common/sm_assert.h"
#include <thread>

#define DENSE_MATRIX_IMPL 1

#if DENSE_MATRIX_IMPL

class CircuitMtx
{
	u64 m_NumNodes = 0;

	Eigen::MatrixXd A; // Coefs matrix
	Eigen::VectorXd x; // Solution
	Eigen::VectorXd b; // Ñurrents

public:

									CircuitMtx();
	void							Solve(bool decompose);	// Ax = b
	double							GetVoltage(int node) const;
	Eigen::MatrixXd&				GetMatrix();
	Eigen::VectorXd&				GetVector();
	Eigen::VectorXd&				GetSolution();
	void							Clear();
	void							Reset();
	void							Resize(u64 NewSize);
	u64								GetNumNodes() const;
	void							Print(std::ostream& os) const;
};

#else
#include "vendor/Eigen/Sparse"

class CircuitMtx
{
private:
	u64 m_NumNodes = 0;

	Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
	Eigen::SparseMatrix<double> A;
	Eigen::VectorXd x;
	Eigen::VectorXd b;

public:
	CircuitMtx();
	void						Solve(); // Ax = b
	double						GetVoltage(int node) const;
	void						Clear();
	void						Reset();
	void						Resize(u64 NewSize);
	u64							GetNumNodes() const;
	void						Print(std::ostream& os) const;

	Eigen::SparseMatrix<double>& GetMatrix();
	Eigen::VectorXd& GetVector();
	Eigen::VectorXd& GetSolution();
};




#endif