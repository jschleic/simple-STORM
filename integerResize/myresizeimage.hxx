/************************************************************************/
/*                                                                      */
/*    Copyright 1998-2010 by Ullrich Koethe and Joachim Schleicher      */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    The VIGRA Website is                                              */
/*        http://hci.iwr.uni-heidelberg.de/vigra/                       */
/*    Please direct questions, bug reports, and contributions to        */
/*        ullrich.koethe@iwr.uni-heidelberg.de    or                    */
/*        vigra@informatik.uni-hamburg.de                               */
/*                                                                      */
/*    Permission is hereby granted, free of charge, to any person       */
/*    obtaining a copy of this software and associated documentation    */
/*    files (the "Software"), to deal in the Software without           */
/*    restriction, including without limitation the rights to use,      */
/*    copy, modify, merge, publish, distribute, sublicense, and/or      */
/*    sell copies of the Software, and to permit persons to whom the    */
/*    Software is furnished to do so, subject to the following          */
/*    conditions:                                                       */
/*                                                                      */
/*    The above copyright notice and this permission notice shall be    */
/*    included in all copies or substantial portions of the             */
/*    Software.                                                         */
/*                                                                      */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND    */
/*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   */
/*    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          */
/*    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       */
/*    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      */
/*    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     */
/*    OTHER DEALINGS IN THE SOFTWARE.                                   */
/*                                                                      */
/************************************************************************/


#include <vigra/basicimage.hxx>

using namespace vigra;

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
inline void myResizeLineLinear(SrcIterator i1, SrcIterator iend, SrcAccessor as, int factor,
                           DestIterator id, DestIterator idend, DestAccessor ad, int additionalBits) {
	//~ std::cout << ibeg << " " << iend << " " << istride << ",,";
	--iend, --idend;
	const int ONE_HALF = 1<<(factor-additionalBits-1);
	for(; i1!=iend; ++i1) {
		for(int j = 0; j < (1<<factor); ++j, ++id) {
			//~ std::cout << i << ":" << (as(i1)>>8) << "=>" << j << " " ;
			typename SrcAccessor::value_type o = (((1<<factor)-j)*as(i1)+j*as(i1,1));
			o += ONE_HALF;
			o >>= factor-additionalBits;
			ad.set(o, id);
		}
	}
	if(additionalBits > 0) {
		ad.set(as(iend)<<additionalBits, idend);
	} else {
		ad.set( (as(iend) + (1<<(-additionalBits-1)))>>(-additionalBits), idend);
	}
	
	//~ std::cout << std::endl;
	return;
}

// --
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
inline void myResizeImageLinear(SrcIterator is, SrcIterator iend, SrcAccessor sa,
		const int factor, DestIterator id, DestIterator idend, DestAccessor da) {

    typedef typename SrcAccessor::value_type SRCVT;
    typedef SRCVT TMPTYPE;

    int w = iend.x - is.x;
    int h = iend.y - is.y;

    int wnew_dest = idend.x - id.x;
    int hnew_dest = idend.y - id.y;
    	
	int wnew = factor*(w-1)+1;
	int hnew = factor*(h-1)+1;
	
	vigra_precondition((wnew==wnew_dest) && (hnew==hnew_dest), "Dest Image dimensions must match factor*SrcImage");

	BasicImage<SRCVT> imgTmp (w,hnew);
	int lnfac = log2(factor); // factor has to be 2**lnfac, only powers of two supported!

    typedef BasicImage<TMPTYPE> TmpImage;
    typedef typename TmpImage::traverser TmpImageIterator;
    typename BasicImage<TMPTYPE>::Iterator yt = imgTmp.upperLeft();

	for(int x = 0; x < w; ++x, ++is.x, ++yt.x) {  // increase height, calculate exact
        typename SrcIterator::column_iterator c1 = is.columnIterator();
        typename TmpImageIterator::column_iterator ct = yt.columnIterator();
		
		myResizeLineLinear( c1, c1 + h, sa, lnfac,
							ct, ct + hnew, imgTmp.accessor(), lnfac);
	}

    yt = imgTmp.upperLeft();
	
	for(int y = 0; y < hnew; ++y, ++yt.y, ++id.y) {  // increase width, round to integers again
        typename DestIterator::row_iterator rd = id.rowIterator();
        typename TmpImageIterator::row_iterator rt = yt.rowIterator();
		myResizeLineLinear(rt, rt + w, imgTmp.accessor(), lnfac,
                                          rd, rd + wnew, da, -lnfac);
	}
}

// --
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
inline
void
myResizeImageLinear(triple<SrcIterator, SrcIterator, SrcAccessor> src, int factor,
                               triple<DestIterator, DestIterator, DestAccessor> dest)
{
    myResizeImageLinear(src.first, src.second, src.third, factor,
                                   dest.first, dest.second, dest.third);
}
