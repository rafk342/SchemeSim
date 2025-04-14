#pragma once
#include <iostream>
#include <format>
//#define  EIGEN_DONT_PARALLELIZE
#include "vendor/Eigen/Dense"


class CircuitMtx
{
	u64 m_NumNodes = 0;

	Eigen::MatrixXd A; // Coefs matrix
	Eigen::VectorXd x; // Solution
	Eigen::VectorXd b; // Ñurrents

public:

									CircuitMtx();
	void							Solve();	// Ax = b
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

