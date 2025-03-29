#include "matrix.h"

CircuitMtx::CircuitMtx()
{
	//Reset();
}

void CircuitMtx::Solve()
{
	x = A.colPivHouseholderQr().solve(b);
}

double CircuitMtx::GetVoltage(int node) const { return x(node); }
Eigen::MatrixXd& CircuitMtx::GetMatrix() { return A; }
Eigen::VectorXd& CircuitMtx::GetVector() { return b; }
Eigen::VectorXd& CircuitMtx::GetSolution() { return x; }
u64 CircuitMtx::GetNumNodes() const { return m_NumNodes; }

void CircuitMtx::Clear()
{
	A.setZero();
	x.setZero();
	b.setZero();
}

void CircuitMtx::Reset()
{
	Resize(0);
}

void CircuitMtx::Resize(u64 NewSize)
{
	if (m_NumNodes == NewSize)
		return;

	if (NewSize == 0)
	{
		m_NumNodes = 0;
		A.resize(0, 0);
		x.resize(0);
		b.resize(0);

		return;
	}

	Eigen::MatrixXd newA = Eigen::MatrixXd::Zero(NewSize, NewSize);
	Eigen::VectorXd newX = Eigen::VectorXd::Zero(NewSize);
	Eigen::VectorXd newB = Eigen::VectorXd::Zero(NewSize);

	u64 minSize = std::min(m_NumNodes, NewSize);
	newA.block(0, 0, minSize, minSize) = A.block(0, 0, minSize, minSize);
	newX.head(minSize) = x.head(minSize);
	newB.head(minSize) = b.head(minSize);

	A = std::move(newA);
	x = std::move(newX);
	b = std::move(newB);

	m_NumNodes = NewSize;
}

void CircuitMtx::Print(std::ostream& os) const
{
	os << std::format("Mtx ({}x{}) | Vector x | Vector b\n", A.rows(), A.cols());
	os << "------------------------------------------\n";
	for (int i = 0; i < A.rows(); i++)
	{
		std::string row;
		for (int j = 0; j < A.cols(); j++)
		{
			row += std::format("{:<7.3f} ", A(i, j));
		}

		row += std::format(" | {:<7.3f}", x(i));
		row += std::format(" | {:<7.3f}", b(i));
		os << row << '\n';
	}
}
