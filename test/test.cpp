/************************************************************************/
/*                                                                      */
/*                  ANALYSIS OF STORM DATA                              */
/*                                                                      */
/*         Copyright 2010 by Joachim Schleicher and Ullrich Koethe      */
/*                                                                      */
/*    Please direct questions, bug reports, and contributions to        */
/*    joachim.schleicher@iwr.uni-heidelberg.de                          */
/************************************************************************/


#include <iostream>
#include <vector>
#include <vigra/tinyvector.hxx>
#include <vigra/impex.hxx>
#include <vigra/affine_registration.hxx>

using namespace std;
using namespace vigra;

int main() {
	//~ std::vector<TinyVector<float, 2> > p1;
	//~ p1.push_back(TinyVector<float,2>(270.38625, 102.79875));
	//~ p1.push_back(TinyVector<float,2>(252.73125, 36.8275));
	//~ p1.push_back(TinyVector<float,2>(470.75, 192.335));
	//~ p1.push_back(TinyVector<float,2>(79.4725, 190.50875));
	//~ p1.push_back(TinyVector<float,2>(214.87625, 384.14625));
	//~ p1.push_back(TinyVector<float,2>(133.3525, 454.08875));

	//~ std::vector<TinyVector<float, 2> > p2;
	//~ p2.push_back(TinyVector<float,2>(269.24875, 104.25));
	//~ p2.push_back(TinyVector<float,2>(251.58875, 38.00125));
	//~ p2.push_back(TinyVector<float,2>(470.385, 194.07));
	//~ p2.push_back(TinyVector<float,2>(77.765, 192.23875));
	//~ p2.push_back(TinyVector<float,2>(213.625, 386.49875));
	//~ p2.push_back(TinyVector<float,2>(131.745, 456.74375));
	
	std::vector<linalg::Matrix<double> > p1;
	std::vector<linalg::Matrix<double> > p2;
	
	linalg::Matrix<double> a(2,1);
	a(0,0) = 270.38625;
	a(1,0) = 102.79875;
	p1.push_back(a);

	a(0,0) = 470.75;
	a(1,0) = 192.335;
	p1.push_back(a);

	a(0,0) = 133.3525;
	a(1,0) = 454.08875;
	p1.push_back(a);
	

	a(0,0) = 269.24875;
	a(1,0) = 104.25;
	p2.push_back(a);

	a(0,0) = 470.385;
	a(1,0) = 194.07;
	p2.push_back(a);

	a(0,0) = 131.745;
	a(1,0) = 456.74375;
	p2.push_back(a);

	linalg::TemporaryMatrix<double> matrix = affineMatrix2DFromCorrespondingPoints(p1.begin(), p1.end(), p2.begin());
	
	cout << matrix << endl;
}
