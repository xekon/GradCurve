// VapourSynth port by HolyWu
//
// Copyright (c) 2002 Tom Barry.  All rights reserved.
//      trbarry@trbarry.com
//  modified by Foxyshadis
//      foxyshadis@hotmail.com
//  modified by Firesledge
//      http://ldesoras.free.fr
//  modified by LaTo INV.
//      http://forum.doom9.org/member.php?u=131032
// Requires Avisynth source code to compile for Avisynth
// Avisynth Copyright 2000 Ben Rudiak-Gould.
//      http://www.math.berkeley.edu/~benrg/avisynth.html
/////////////////////////////////////////////////////////////////////////////
//
//  This file is subject to the terms of the GNU General Public License as
//  published by the Free Software Foundation.  A copy of this license is
//  included with this software distribution in the file COPYING.  If you
//  do not have a copy, you may obtain a copy by writing to the Free
//  Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details
//  
//  Also, this program is "Philanthropy-Ware".  That is, if you like it and 
//  feel the need to reward or inspire the author then please feel free (but
//  not obligated) to consider joining or donating to the Electronic Frontier
//  Foundation. This will help keep cyber space free of barbed wire and bullsh*t.  
//
/////////////////////////////////////////////////////////////////////////////
// Change Log
//
// Date          Version  Developer      Changes
//
// 07 May 2003   1.0.0.0  Tom Barry      New Release
// 01 Jun 2006   1.1.0.0  Foxyshadis     Chroma noise, constant seed
// 06 Jun 2006   1.2.0.0  Foxyshadis     Supports YUY2, RGB. Fix cache mess.
// 10 Jun 2006   1.3.0.0  Foxyshadis     Crashfix, noisegen optimization
// 11 Nov 2006   1.4.0.0  Foxyshadis     Constant replaces seed, seed repeatable
// 07 May 2010   1.5.0.0  Foxyshadis     Limit the initial seed generation to fix memory issues.
// 13 May 2010   1.5.1.0  Firesledge     The source code compiles on Visual C++ versions older than 2008
// 26 Oct 2011   1.5.2.0  Firesledge     Removed the SSE2 requirement.
// 26 Oct 2011   1.5.3.0  Firesledge     Fixed coloring and bluring in RGB24 mode.
// 27 Oct 2011   1.5.4.0  Firesledge     Fixed bad pixels on the last line in YV12 mode when constant=true,
//                                       fixed potential problems with frame width > 4096 pixels
//                                       and fixed several other minor things.
// 28 Oct 2011   1.6.0.0  LaTo INV.      Added SSE2 code (50% faster than MMX).
// 29 Oct 2011   1.6.1.0  LaTo INV.      Automatic switch to MMX if SSE2 is not supported by the CPU.
// 16 Aug 2012   1.7.0.0  Firesledge     Supports Y8, YV16, YV24 and YV411 colorspaces.
//
/////////////////////////////////////////////////////////////////////////////

//#include <windows.h>
//#include <commctrl.h>
//#include <stdio.h>
//#include <math.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <ctime>
#include <vector>
#include <vapoursynth/VapourSynth.h>
#include <vapoursynth/VSHelper.h>

// max # of noise planes
#define MAXP 2

// offset in pixels of the fake plane MAXP relative to plane MAXP-1
#define OFFSET_FAKEPLANE 32

long *rgblab; //LUT Lab
long *labrgb; //LUT Lab

struct GradCurveData {
	VSNodeRef * node;
	const VSVideoInfo * vi;
	bool constant;
	int64_t idum;
	int nStride[MAXP], nHeight[MAXP], nSize[MAXP];
	int storedFrames;
	std::vector<uint8_t> pNoiseSeeds;
	std::vector<int16_t> pN[MAXP];
	std::vector<float> pNF[MAXP];
	bool process[3];
	float lower[3], upper[3];
	//mine
	//IFilterPreview		*ifp; //vdub
	long rvalue[3][256];
	int gvalue[3][256];
	int bvalue[256];
	int ovalue[5][256];
	int value;
	int space_mode;
	int channel_mode;
	int proces;
	int xl;
	int yl;
	int offset;
	char filename[1024];
	int filter;
	bool Labprecalc;
	int laboff;
	int drwmode[5];
	int drwpoint[5][16][2];
	int poic[5];
	int cp;
	bool psel;
	int noise, scale, block;
	int iterTimesTwiceScaling;
	float * srcInterleaved, *dstInterleaved, *buffer;
	float oldr, oldb, oldg, medr, medb, medg;
	char gamma[10];//windows.h
};

void PreCalcLut()
{
	long count;
	long x;
	long y;
	long z;
	long rr;
	long gg;
	long bb;
	long r1;
	long g1;
	long b1;
	int r;
	int g;
	int b;

	rgblab = new long[16777216];
	labrgb = new long[16777216];

	count = 0;
	for (r = 0; r<256; r++) {
		for (g = 0; g<256; g++) {
			for (b = 0; b<256; b++) {
				if (r > 10) { rr = long(pow(((r << 4) + 224.4), (2.4))); }
				else { rr = long((r << 4)*9987.749); }
				if (g > 10) { gg = long(pow(((g << 4) + 224.4), (2.4))); }
				else { gg = long((g << 4)*9987.749); }
				if (b > 10) { bb = long(pow(((b << 4) + 224.4), (2.4))); }
				else { bb = long((b << 4)*9987.749); }
				x = long((rr + 6.38287545) / 12.7657509 + (gg + 7.36187255) / 14.7237451 + (bb + 14.58712555) / 29.1742511);
				y = long((rr + 12.37891725) / 24.7578345 + (gg + 3.68093628) / 7.36187256 + (bb + 36.4678139) / 72.9356278);
				z = long((rr + 136.1678335) / 272.335667 + (gg + 22.0856177) / 44.1712354 + (bb + 2.76970661) / 5.53941322);
				//XYZ to Lab
				if (x>841776) { rr = long(pow((x), (0.33333333333333333333333333333333))*21.9122842); }
				else { rr = long((x + 610.28989295) / 1220.5797859 + 1379.3103448275862068965517241379); }
				if (y>885644) { gg = long(pow((y), (0.33333333333333333333333333333333))*21.5443498); }
				else { gg = long((y + 642.0927467) / 1284.1854934 + 1379.3103448275862068965517241379); }
				if (z>964440) { bb = long(pow((z), (0.33333333333333333333333333333333))*20.9408726); }
				else { bb = long((z + 699.1298454) / 1398.2596908 + 1379.3103448275862068965517241379); }
				x = long(((gg + 16.90331) / 33.806620) - 40.8);
				y = long(((rr - gg + 7.23208898) / 14.46417796) + 119.167434);
				z = long(((gg - bb + 19.837527645) / 39.67505529) + 135.936123);
				rgblab[count] = ((x << 16) + (y << 8) + z);
				count++;
			}
		}
	}
	count = 0;
	for (x = 0; x<256; x++) {
		for (y = 0; y<256; y++) {
			for (z = 0; z<256; z++) {
				gg = x * 50 + 2040;
				rr = long(y*21.392519204 - 2549.29163142 + gg);
				bb = long(gg - z*58.67940678 + 7976.6510628);
				if (gg>3060) { g1 = long(gg*gg / 32352.25239*gg); }
				else { g1 = long(x*43413.9788); }
				if (rr>3060) { r1 = long(rr*rr / 34038.16258*rr); }
				else { r1 = long(rr*825.27369 - 1683558); }
				if (bb>3060) { b1 = long(bb*bb / 29712.85911*bb); }
				else { b1 = long(bb*945.40885 - 1928634); }
				//XYZ to RGB
				rr = long(r1*16.20355 + g1*-7.6863 + b1*-2.492855);
				gg = long(r1*-4.84629 + g1*9.37995 + b1*0.2077785);
				bb = long(r1*0.278176 + g1*-1.01998 + b1*5.28535);
				if (rr>1565400) { r = int((pow((rr), (0.41666666666666666666666666666667)) + 7.8297554795) / 15.659510959 - 13.996); }
				else { r = int((rr + 75881.7458872) / 151763.4917744); }
				if (gg>1565400) { g = int((pow((gg), (0.41666666666666666666666666666667)) + 7.8297554795) / 15.659510959 - 14.019); }
				else { g = int((gg + 75881.7458872) / 151763.4917744); }
				if (bb>1565400) { b = int((pow((bb), (0.41666666666666666666666666666667)) + 7.8297554795) / 15.659510959 - 13.990); }
				else { b = int((bb + 75881.7458872) / 151763.4917744); }
				if (r<0) { r = 0; }
				else if (r>255) { r = 255; }
				if (g<0) { g = 0; }
				else if (g>255) { g = 255; }
				if (b<0) { b = 0; }
				else if (b>255) { b = 255; }
				labrgb[count] = ((r << 16) + (g << 8) + b);
				count++;
			}
		}
	}
}

void CalcCurve(GradCurveData * d)
{
	int c1;
	int c2;
	int dx;
	int dy;
	int dxg;
	int dyg;
	int i;
	int j;
	int vy;
	double div;
	double inc;
	double ofs;
	double ga;
	double x[16][16];
	double y[16];
	double a[16];
	double b[16];
	double c[16];

	if (d->drwpoint[d->channel_mode][0][0]>0) { for (c2 = 0; c2<d->drwpoint[d->channel_mode][0][0]; c2++) { d->ovalue[d->channel_mode][c2] = d->drwpoint[d->channel_mode][0][1]; } }
	switch (d->drwmode[d->channel_mode]) {
	case 1:	//linear mode
		for (c1 = 0; c1<(d->poic[d->channel_mode] - 1); c1++) {
			div = (d->drwpoint[d->channel_mode][(c1 + 1)][0] - d->drwpoint[d->channel_mode][c1][0]);
			inc = (d->drwpoint[d->channel_mode][(c1 + 1)][1] - d->drwpoint[d->channel_mode][c1][1]) / div;
			ofs = d->drwpoint[d->channel_mode][c1][1] - inc*d->drwpoint[d->channel_mode][c1][0];
			for (c2 = d->drwpoint[d->channel_mode][c1][0]; c2<(d->drwpoint[d->channel_mode][(c1 + 1)][0] + 1); c2++)
			{
				d->ovalue[d->channel_mode][c2] = int(c2*inc + ofs + 0.5);
			}
		}
		break;
	case 2: //spline mode
		for (i = 0; i<16; i++) {	 //clear tables
			for (j = 0; j<16; j++) { x[i][j] = 0; }
			y[i] = 0;
			a[i] = 0;
			b[i] = 0;
			c[i] = 0;
		}

		if (d->poic[d->channel_mode]>3) {	//curve has more than 3 coordinates
			j = d->poic[d->channel_mode] - 3;		 //fill the matrix needed to calculate the b coefficients of the cubic functions an*x^3+bn*x^2+cn*x+dn
			x[0][0] = double(2 * (d->drwpoint[d->channel_mode][2][0] - d->drwpoint[d->channel_mode][0][0]));
			x[0][1] = double((d->drwpoint[d->channel_mode][2][0] - d->drwpoint[d->channel_mode][1][0]));
			y[0] = 3 * (double(d->drwpoint[d->channel_mode][2][1] - d->drwpoint[d->channel_mode][1][1]) / double(d->drwpoint[d->channel_mode][2][0] - d->drwpoint[d->channel_mode][1][0]) - double(d->drwpoint[d->channel_mode][1][1] - d->drwpoint[d->channel_mode][0][1]) / double(d->drwpoint[d->channel_mode][1][0] - d->drwpoint[d->channel_mode][0][0]));
			for (i = 1; i<j; i++) {
				x[i][i - 1] = double((d->drwpoint[d->channel_mode][i + 1][0] - d->drwpoint[d->channel_mode][i][0]));
				x[i][i] = double(2 * (d->drwpoint[d->channel_mode][i + 2][0] - d->drwpoint[d->channel_mode][i][0]));
				x[i][i + 1] = double((d->drwpoint[d->channel_mode][i + 2][0] - d->drwpoint[d->channel_mode][i + 1][0]));
				y[i] = 3 * (double(d->drwpoint[d->channel_mode][i + 2][1] - d->drwpoint[d->channel_mode][i + 1][1]) / double(d->drwpoint[d->channel_mode][i + 2][0] - d->drwpoint[d->channel_mode][i + 1][0]) - double(d->drwpoint[d->channel_mode][i + 1][1] - d->drwpoint[d->channel_mode][i][1]) / double(d->drwpoint[d->channel_mode][i + 1][0] - d->drwpoint[d->channel_mode][i][0]));
			}
			x[j][j - 1] = double(d->drwpoint[d->channel_mode][j + 1][0] - d->drwpoint[d->channel_mode][j][0]);
			x[j][j] = double(2 * (d->drwpoint[d->channel_mode][j + 2][0] - d->drwpoint[d->channel_mode][j][0]));
			y[j] = 3 * (double(d->drwpoint[d->channel_mode][j + 2][1] - d->drwpoint[d->channel_mode][j + 1][1]) / double(d->drwpoint[d->channel_mode][j + 2][0] - d->drwpoint[d->channel_mode][j + 1][0]) - double(d->drwpoint[d->channel_mode][j + 1][1] - d->drwpoint[d->channel_mode][j][1]) / double(d->drwpoint[d->channel_mode][j + 1][0] - d->drwpoint[d->channel_mode][j][0]));

			for (i = 0; i<d->poic[d->channel_mode] - 3; i++) {	 //resolve the matrix to get the b coefficients
				div = x[i + 1][i] / x[i][i];
				x[i + 1][i] = x[i + 1][i] - x[i][i] * div;
				x[i + 1][i + 1] = x[i + 1][i + 1] - x[i][i + 1] * div;
				x[i + 1][i + 2] = x[i + 1][i + 2] - x[i][i + 2] * div;
				y[i + 1] = y[i + 1] - y[i] * div;
			}
			b[d->poic[d->channel_mode] - 2] = y[d->poic[d->channel_mode] - 3] / x[d->poic[d->channel_mode] - 3][d->poic[d->channel_mode] - 3];	//last b coefficient 
			for (i = d->poic[d->channel_mode] - 3; i>0; i--) { b[i] = (y[i - 1] - x[i - 1][i] * b[i + 1]) / x[i - 1][i - 1]; } // backward subsitution to get the rest of the the b coefficients
		}
		else if (d->poic[d->channel_mode] == 3) { //curve has 3 coordinates
			b[1] = 3 * (double(d->drwpoint[d->channel_mode][2][1] - d->drwpoint[d->channel_mode][1][1]) / double(d->drwpoint[d->channel_mode][2][0] - d->drwpoint[d->channel_mode][1][0]) - double(d->drwpoint[d->channel_mode][1][1] - d->drwpoint[d->channel_mode][0][1]) / double(d->drwpoint[d->channel_mode][1][0] - d->drwpoint[d->channel_mode][0][0])) / double(2 * (d->drwpoint[d->channel_mode][2][0] - d->drwpoint[d->channel_mode][0][0]));
		}

		for (c2 = 0; c2<(d->poic[d->channel_mode] - 1); c2++) {	 //get the a and c coefficients
			a[c2] = (double(b[c2 + 1] - b[c2]) / double(3 * (d->drwpoint[d->channel_mode][c2 + 1][0] - d->drwpoint[d->channel_mode][c2][0])));
			c[c2] = double(d->drwpoint[d->channel_mode][c2 + 1][1] - d->drwpoint[d->channel_mode][c2][1]) / double(d->drwpoint[d->channel_mode][c2 + 1][0] - d->drwpoint[d->channel_mode][c2][0]) - double(b[c2 + 1] - b[c2])*double(d->drwpoint[d->channel_mode][c2 + 1][0] - d->drwpoint[d->channel_mode][c2][0]) / 3 - b[c2] * (d->drwpoint[d->channel_mode][c2 + 1][0] - d->drwpoint[d->channel_mode][c2][0]);
		}
		for (c1 = 0; c1<(d->poic[d->channel_mode] - 1); c1++) {	 //calculate the y values of the spline curve
			for (c2 = d->drwpoint[d->channel_mode][(c1)][0]; c2<(d->drwpoint[d->channel_mode][(c1 + 1)][0] + 1); c2++) {
				vy = int(0.5 + a[c1] * (c2 - d->drwpoint[d->channel_mode][c1][0])*(c2 - d->drwpoint[d->channel_mode][c1][0])*(c2 - d->drwpoint[d->channel_mode][c1][0]) + b[c1] * (c2 - d->drwpoint[d->channel_mode][c1][0])*(c2 - d->drwpoint[d->channel_mode][c1][0]) + c[c1] * (c2 - d->drwpoint[d->channel_mode][c1][0]) + d->drwpoint[d->channel_mode][c1][1]);
				if (vy>255) { d->ovalue[d->channel_mode][c2] = 255; }
				else if (vy<0) { d->ovalue[d->channel_mode][c2] = 0; }
				else { d->ovalue[d->channel_mode][c2] = vy; }
			}
		}
		break;
	case 3: //gamma mode
		dx = d->drwpoint[d->channel_mode][2][0] - d->drwpoint[d->channel_mode][0][0];
		dy = d->drwpoint[d->channel_mode][2][1] - d->drwpoint[d->channel_mode][0][1];
		dxg = d->drwpoint[d->channel_mode][1][0] - d->drwpoint[d->channel_mode][0][0];
		dyg = d->drwpoint[d->channel_mode][1][1] - d->drwpoint[d->channel_mode][0][1];
		ga = log(double(dyg) / double(dy)) / log(double(dxg) / double(dx));
		snprintf(d->gamma, 10, "%.3lf", (1 / ga));
		for (c1 = 0; c1<dx + 1; c1++) {
			d->ovalue[d->channel_mode][c1 + d->drwpoint[d->channel_mode][0][0]] = int(0.5 + dy*(pow((double(c1) / dx), (ga)))) + d->drwpoint[d->channel_mode][0][1];
		}
		break;
	}
	if (d->drwpoint[d->channel_mode][((d->poic[d->channel_mode]) - 1)][0]<255) { for (c2 = d->drwpoint[d->channel_mode][((d->poic[d->channel_mode]) - 1)][0]; c2<256; c2++) { d->ovalue[d->channel_mode][c2] = d->drwpoint[d->channel_mode][(d->poic[d->channel_mode] - 1)][1]; } }
	switch (d->channel_mode) { //for faster RGB modes
	case 0:
		for (i = 0; i<256; i++) {
			d->rvalue[0][i] = (d->ovalue[0][i] << 16);
			d->rvalue[2][i] = (d->ovalue[0][i] - i) << 16;
			d->gvalue[0][i] = (d->ovalue[0][i] << 8);
			d->gvalue[2][i] = (d->ovalue[0][i] - i) << 8;
			d->bvalue[i] = d->ovalue[0][i] - i;
		}
		break;
	case 1:
		for (i = 0; i<256; i++) { d->rvalue[1][i] = (d->ovalue[1][i] << 16); }
		break;
	case 2:
		for (i = 0; i<256; i++) { d->gvalue[1][i] = (d->ovalue[2][i] << 8); }
		break;
	}
}

void ImportCurve(GradCurveData * d) // import curves
{
	FILE *pFile;
	int i;
	int j;
	int stor[1280];
	int temp[1280];
	long lSize = 0;//xekon set 0 default, clear uninitialized error
	int beg;
	int cv;
	int count = 0;//xekon set 0 default, clear uninitialized error
	int noocur = 0;//xekon set 0 default, clear uninitialized error
	int curpos;
	int cordpos;
	int curposnext = 0;//xekon set 0 default, clear uninitialized error
	int cordcount;
	int cmtmp;
	int drwmodtmp;
	int pictmp;
	int drwtmp[16][2];
	bool nrf;
	int gma;
	curpos = 0;
	cordpos = 7;
	cordcount = 0;
	gma = 1;
	nrf = false;

	for (i = 0; i<5; i++) { d->drwmode[i] = 0; }

	if (d->filter == 2)	// *.acv
	{
		pFile = fopen(d->filename, "rb");
		if (pFile == NULL) {
			//vsapi->setError(out, "GradCurve: Error opening file"); 
		}
		else
		{
			fseek(pFile, 0, SEEK_END);
			lSize = ftell(pFile);
			rewind(pFile);
			for (i = 0; (i < lSize) && (feof(pFile) == 0); i++)	 //read the file and store the coordinates
			{
				cv = fgetc(pFile);
				if (i == 3) {
					noocur = cv;
					if (noocur>5) { noocur = 5; }
					curpos = 0;
				}
				if (i == 5) {
					d->poic[curpos] = cv;
					if (noocur >= (curpos + 1))
					{
						curposnext = i + d->poic[curpos] * 4 + 2;
						curpos++;
					}
				}
				if (i == curposnext) {
					d->poic[curpos] = cv;
					if (noocur >= (curpos + 1))
					{
						curposnext = i + d->poic[curpos] * 4 + 2;
						if (d->poic[curpos - 1]>16) { d->poic[curpos - 1] = 16; }
						curpos++;
						cordcount = 0;
						cordpos = i + 2;
					}
				}
				if (i == cordpos) {
					d->drwpoint[curpos - 1][cordcount][1] = cv;
				}
				if (i == (cordpos + 2)) {
					d->drwpoint[curpos - 1][cordcount][0] = cv;
					if (cordcount<15) { cordcount++; }
					cordpos = cordpos + 4;
				}
			}
			fclose(pFile);
			if (noocur<5) { //fill empty curves if acv does contain less than 5 curves
				for (i = noocur; i<5; i++)
				{
					d->poic[i] = 2;
					d->drwpoint[i][0][0] = 0;
					d->drwpoint[i][0][1] = 0;
					d->drwpoint[i][1][0] = 255;
					d->drwpoint[i][1][1] = 255;
				}
				noocur = 5;
			}
			d->cp = 0;
			cmtmp = d->channel_mode;
			for (i = 0; i<5; i++) {	// calculate curve values
				d->drwmode[i] = 2;
				d->channel_mode = i;
				CalcCurve(d);
			}
			d->channel_mode = cmtmp;
			nrf = true;
		}
	}
	if (d->filter == 3) { // *.csv
		pFile = fopen(d->filename, "r");
		if (pFile == NULL) { //vsapi->setError(out, "GradCurve: Error opening file");
		}
		else
		{
			fseek(pFile, 0, SEEK_END);
			lSize = ftell(pFile);
			rewind(pFile);
			// xekon temp comment out, possible error/bug
			//for (i = 0; (i < 1280) && (feof(pFile) == 0); i++)
			//{
			//	fscanf(pFile, "%d", & stor[i]);
			//}
			// end xekon
			fclose(pFile);
			lSize = lSize / 4;
		}
	}
	else if (d->filter == 4 || d->filter == 5) {	 // *.crv *.map
		if (d->filter == 4) { beg = 64; }
		else { beg = 320; }
		curpos = -1;
		curposnext = 65530;
		cordpos = beg + 6;
		pFile = fopen(d->filename, "rb");
		if (pFile == NULL) { //vsapi->setError(out, "GradCurve: Error opening file");
		}
		else
		{
			fseek(pFile, 0, SEEK_END);
			lSize = ftell(pFile);
			rewind(pFile);
			for (i = 0; (i < lSize) && (feof(pFile) == 0); i++)
			{
				cv = fgetc(pFile);
				if (i == beg) {
					curpos++;
					d->drwmode[curpos] = cv;
					curposnext = 65530;
					if (d->drwmode[curpos] == 2 || d->drwmode[curpos] == 0) {
						d->drwmode[curpos] = abs(d->drwmode[curpos] - 2);
					}
				}
				if (i == beg + 1 && d->drwmode[curpos] == 3) { gma = cv; }
				if (i == beg + 2 && d->drwmode[curpos] == 3) { gma = gma + (cv << 8); }
				if (i == beg + 5) {
					d->poic[curpos] = cv;
					cordpos = i + 1;
					curposnext = i + d->poic[curpos] * 2 + 1;
					if (curpos<4) { beg = i + d->poic[curpos] * 2 + 257; }
					cordcount = 0;
					count = 0;
					if (d->poic[curpos]>16) { d->poic[curpos] = 16; } // limit to 16 points
				}
				if (i >= curposnext) { // read raw curve data
					cordpos = 0;
					if (count<256) { d->ovalue[curpos][count] = cv; }
					count++;
				}
				if (i == cordpos) {
					if (d->drwmode[curpos] == 3 && cordcount == 1) {
						if (gma>250) { d->drwpoint[curpos][cordcount][0] = 64; }
						else if (gma<50) { d->drwpoint[curpos][cordcount][0] = 192; }
						else { d->drwpoint[curpos][cordcount][0] = 128; }
						d->drwpoint[curpos][cordcount][1] = int(pow(float(d->drwpoint[curpos][cordcount][0]) / 256, 100 / float(gma)) * 256 + 0.5);
						cordcount++;
						d->poic[curpos]++;
					}
					d->drwpoint[curpos][cordcount][0] = cv;
				}
				if (i == cordpos + 1) {
					d->drwpoint[curpos][cordcount][1] = cv;
					if (cordcount<d->poic[curpos] - 1 && cordcount<15) { cordcount++; } // limit to 16 points
					cordpos = cordpos + 2;
				}
			}
			fclose(pFile);
		}
		if (d->filter == 5) {	 //*.map exchange 4<->0
			drwmodtmp = d->drwmode[4];
			pictmp = d->poic[4];
			for (i = 0; i<pictmp; i++) {
				drwtmp[i][0] = d->drwpoint[4][i][0];
				drwtmp[i][1] = d->drwpoint[4][i][1];
			}
			for (j = 4; j>0; j--) {
				for (i = 0; i<d->poic[j - 1]; i++) {
					d->drwpoint[j][i][0] = d->drwpoint[j - 1][i][0];
					d->drwpoint[j][i][1] = d->drwpoint[j - 1][i][1];
				}
				d->poic[j] = d->poic[j - 1];
				d->drwmode[j] = d->drwmode[j - 1];
			}
			for (i = 0; i<pictmp; i++) {
				d->drwpoint[0][i][0] = drwtmp[i][0];
				d->drwpoint[0][i][1] = drwtmp[i][1];
			}
			d->poic[0] = pictmp;
			d->drwmode[0] = drwmodtmp;
			for (i = 0; i<256; i++) { temp[i] = d->ovalue[4][i]; }
			for (j = 4; j>0; j--) {
				for (i = 0; i<256; i++) { d->ovalue[j][i] = d->ovalue[j - 1][i]; }
			}
			for (i = 0; i<256; i++) { d->ovalue[0][i] = temp[i]; }
		}
		cmtmp = d->channel_mode;
		for (i = 0; i<5; i++) {	// calculate curve values		
			d->channel_mode = i;
			if (d->drwmode[i] != 0) { CalcCurve(d); }
		}
		d->channel_mode = cmtmp;
		d->cp = 0;
		nrf = true;
	}
	else if (d->filter == 6)	// *.amp Smartvurve hsv
	{
		pFile = fopen(d->filename, "rb");
		if (pFile == NULL) { //vsapi->setError(out, "GradCurve: Error opening file");
		}
		else
		{
			fseek(pFile, 0, SEEK_END);
			lSize = ftell(pFile);
			rewind(pFile);
			for (i = 0; (i < 768) && (feof(pFile) == 0); i++)
			{
				if (i<256)
				{
					stor[i + 512] = fgetc(pFile);
				}
				if (i>255 && i <512)
				{
					stor[i] = fgetc(pFile);
				}
				if (i>511)
				{
					stor[i - 512] = fgetc(pFile);
				}
			}
			fclose(pFile);
			lSize = 768;
		}
	}
	else
	{
		pFile = fopen(d->filename, "rb"); // *.amp
		if (pFile == NULL) { //vsapi->setError(out, "GradCurve: Error opening file");
		}
		else
		{
			fseek(pFile, 0, SEEK_END);
			lSize = ftell(pFile);
			rewind(pFile);
			for (i = 0; (i < 1280) && (feof(pFile) == 0); i++)
			{
				stor[i] = fgetc(pFile);
			}
			fclose(pFile);
		}
	}
	if (nrf == false) { //fill curves for non coordinates file types
		if (lSize > 768) {
			for (i = 0; i < 256; i++) {
				d->ovalue[0][i] = stor[i];
				d->rvalue[0][i] = (d->ovalue[0][i] << 16);
				d->rvalue[2][i] = (d->ovalue[0][i] - i) << 16;
				d->gvalue[0][i] = (d->ovalue[0][i] << 8);
				d->gvalue[2][i] = (d->ovalue[0][i] - i) << 8;
				d->bvalue[i] = d->ovalue[0][i] - i;
			}
			for (i = 256; i < 512; i++) {
				d->ovalue[1][(i - 256)] = stor[i];
				d->rvalue[1][(i - 256)] = (d->ovalue[1][(i - 256)] << 16);
			}
			for (i = 512; i < 768; i++) {
				d->ovalue[2][(i - 512)] = stor[i];
				d->gvalue[1][(i - 512)] = (d->ovalue[2][(i - 512)] << 8);
			}
			for (i = 768; i < 1024; i++) { d->ovalue[3][(i - 768)] = stor[i]; }
			for (i = 1024; i < 1280; i++) { d->ovalue[4][(i - 1024)] = stor[i]; }
		}
		if (lSize < 769 && lSize > 256) {
			for (i = 0; i < 256; i++) {
				d->ovalue[1][i] = stor[i];
				d->rvalue[1][i] = (d->ovalue[1][i] << 16);
			}
			for (i = 256; i < 512; i++) {
				d->ovalue[2][(i - 256)] = stor[i];
				d->gvalue[1][(i - 256)] = (d->ovalue[2][(i - 256)] << 8);
			}
			for (i = 512; i < 768; i++) { d->ovalue[3][(i - 512)] = stor[i]; }
		}
		if (lSize < 257 && lSize > 0) {
			for (i = 0; i < 256; i++) {
				d->ovalue[0][i] = stor[i];
				d->rvalue[0][i] = (d->ovalue[0][i] << 16);
				d->rvalue[2][i] = (d->ovalue[0][i] - i) << 16;
				d->gvalue[0][i] = (d->ovalue[0][i] << 8);
				d->gvalue[2][i] = (d->ovalue[0][i] - i) << 8;
				d->bvalue[i] = d->ovalue[0][i] - i;
			}
		}
		for (i = 0; i<5; i++) {
			d->drwmode[i] = 0;
			d->poic[i] = 2;
			d->drwpoint[i][0][0] = 0;
			d->drwpoint[i][0][1] = 0;
			d->drwpoint[i][1][0] = 255;
			d->drwpoint[i][1][1] = 255;
		}
	}
}

static void VS_CC GradCurveInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi) {
	GradCurveData * d = static_cast<GradCurveData *>(*instanceData);
	vsapi->setVideoInfo(d->vi, 1, node);
}

static const VSFrameRef *VS_CC GradCurveGetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi) {
	GradCurveData * d = static_cast<GradCurveData *>(*instanceData);

	if (activationReason == arInitial) {
		vsapi->requestFrameFilter(n, d->node, frameCtx);
	}
	else if (activationReason == arAllFramesReady) {
		const VSFrameRef * src = vsapi->getFrameFilter(n, d->node, frameCtx);
		VSFrameRef * dst = vsapi->copyFrame(src, core);

		//vdub startproc()
		ImportCurve(d);
		if (d->Labprecalc == 0 && d->proces == 8) {	// build up the LUT for the Lab process	if it is not precalculated already
			PreCalcLut();
			d->Labprecalc = 1;
		}

		//borrowed and modified from Waifu2x-w2xc
		const int width = vsapi->getFrameWidth(src, 0);
		const int height = vsapi->getFrameHeight(src, 0);
		const int srcStride = vsapi->getStride(src, 0) / sizeof(float);
		const int dstStride = vsapi->getStride(dst, 0) / sizeof(float);
		const float * srcpR = reinterpret_cast<const float *>(vsapi->getReadPtr(src, 0));
		const float * srcpG = reinterpret_cast<const float *>(vsapi->getReadPtr(src, 1));
		const float * srcpB = reinterpret_cast<const float *>(vsapi->getReadPtr(src, 2));
		float * VS_RESTRICT dstpR = reinterpret_cast<float *>(vsapi->getWritePtr(dst, 0));
		float * VS_RESTRICT dstpG = reinterpret_cast<float *>(vsapi->getWritePtr(dst, 1));
		float * VS_RESTRICT dstpB = reinterpret_cast<float *>(vsapi->getWritePtr(dst, 2));

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				const int pos = width * y + x;
				d->srcInterleaved[pos * 3] = srcpR[x];
				d->srcInterleaved[pos * 3 + 1] = srcpG[x];
				d->srcInterleaved[pos * 3 + 2] = srcpB[x];

				d->oldr = srcpR[x];
				d->oldb = srcpB[x];
				d->oldg = srcpG[x];

				d->medr = d->rvalue[1][int(d->oldr)];
				d->medb = d->ovalue[1][int(d->oldb)];
				d->medg = d->gvalue[1][int(d->oldg)];

				dstpR[x] = d->rvalue[0][int(d->medr)];
				dstpB[x] = d->ovalue[0][int(d->medb)];
				dstpG[x] = d->gvalue[0][int(d->medg)];
			}


			srcpR += srcStride;
			srcpG += srcStride;
			srcpB += srcStride;

			dstpR += dstStride;
			dstpG += dstStride;
			dstpB += dstStride;
		}
		//end xekon vdub
		
		vsapi->freeFrame(src);
		
		return dst;		

		//for (int plane = 0; plane < d->vi->format->numPlanes; plane++) {
		//	if (d->process[plane]) {
		//		const int width = vsapi->getFrameWidth(dst, plane);
		//		const int height = vsapi->getFrameHeight(dst, plane);
		//		const int stride = vsapi->getStride(dst, plane);
		//		uint8_t * dstp = vsapi->getWritePtr(dst, plane);
		//		int noisePlane = (d->vi->format->colorFamily == cmRGB) ? 0 : plane;
		//		int noiseOffs = 0;
		//		setRand(&noisePlane, &noiseOffs, n, d); // seeds randomness w/ plane & frame
		//		if (d->vi->format->sampleType == stInteger) {
		//			if (d->vi->format->bitsPerSample == 8)
		//				updateFrame<uint8_t, int8_t>(dstp, width, height, stride, noisePlane, noiseOffs, plane, d);
		//			else
		//				updateFrame<uint16_t, int16_t>(reinterpret_cast<uint16_t *>(dstp), width, height, stride / 2, noisePlane, noiseOffs, plane, d);
		//		}
		//		else {
		//			updateFrame<float, float>(reinterpret_cast<float *>(dstp), width, height, stride / 4, noisePlane, noiseOffs, plane, d);
		//		}
		//	}
		//}

		vsapi->freeFrame(src);
		return dst;
	}

	return nullptr;
}

static void VS_CC GradCurveFree(void *instanceData, VSCore *core, const VSAPI *vsapi) {
	GradCurveData * d = static_cast<GradCurveData *>(instanceData);
	vsapi->freeNode(d->node);
	delete d;
}

static void VS_CC GradCurveCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi) {
	GradCurveData d;
	
	//temp unused chunk, function parameter setting and error testing
	int err;
	strcat(d.filename, (vsapi->propGetData(in, "var", 0, &err)));//hard coded, file path for test
	//float var = static_cast<float>(vsapi->propGetFloat(in, "var", 0, &err));
	//if (err)
	//	var = 1.f;
	//float uvar = static_cast<float>(vsapi->propGetFloat(in, "uvar", 0, &err));
	//const float hcorr = static_cast<float>(vsapi->propGetFloat(in, "hcorr", 0, &err));
	//const float vcorr = static_cast<float>(vsapi->propGetFloat(in, "vcorr", 0, &err));
	//int64_t seed = vsapi->propGetInt(in, "seed", 0, &err);
	//if (err)
	//	seed = -1;
	//d.constant = !!vsapi->propGetInt(in, "constant", 0, &err);
	//if (hcorr < 0.f || hcorr > 1.f || vcorr < 0.f || vcorr > 1.f) {
	//	vsapi->setError(out, "GradCurve: hcorr and vcorr must be between 0.0 and 1.0 (inclusive)");
	//	return;
	//}
	//end unused chunk

	d.node = vsapi->propGetNode(in, "clip", 0, nullptr);
	d.vi = vsapi->getVideoInfo(d.node);

	if (!isConstantFormat(d.vi) || (d.vi->format->sampleType == stInteger && d.vi->format->bitsPerSample > 16) ||
		(d.vi->format->sampleType == stFloat && d.vi->format->bitsPerSample != 32)) {
		vsapi->setError(out, "GradCurve: only constant format 8-16 bits integer and 32 bits float input supported");
		vsapi->freeNode(d.node);
		return;
	}

	//vdub initproc()
	d.Labprecalc = 0;
	for (int i = 0; i<5; i++) {
		d.drwmode[i] = 2;
		d.poic[i] = 2;
		d.drwpoint[i][0][0] = 0;
		d.drwpoint[i][0][1] = 0;
		d.drwpoint[i][1][0] = 255;
		d.drwpoint[i][1][1] = 255;
	}
	d.value = 0;
	d.proces = 0;
	d.xl = 300;
	d.yl = 300;
	d.offset = 0;
	d.psel = false;
	d.cp = 0;
	snprintf(d.gamma, 10, "%.3lf", 1.000);
	for (int i = 0; i<256; i++) {
		d.ovalue[0][i] = i;
		d.rvalue[0][i] = (d.ovalue[0][i] << 16);
		d.rvalue[2][i] = (d.ovalue[0][i] - i) << 16;
		d.gvalue[0][i] = (d.ovalue[0][i] << 8);
		d.gvalue[2][i] = (d.ovalue[0][i] - i) << 8;
		d.bvalue[i] = (d.ovalue[0][i] - i);
		d.ovalue[1][i] = i;
		d.rvalue[1][i] = (d.ovalue[1][i] << 16);
		d.ovalue[2][i] = i;
		d.gvalue[1][i] = (d.ovalue[2][i] << 8);
		d.ovalue[3][i] = i;
		d.ovalue[4][i] = i;
	}
	//scriptconfig()
	d.proces = 1; //hard coded testing, RGB + R/G/B
	d.filter = 2; //hard coded testing, curve file type ACV
				  //d.filename = "D:/enc/032t.acv"; //hard coded testing, curve file
	d.channel_mode = 0;//hard coded, copied from callback() rgb modes
	d.offset = 0;//hard coded, copied from callback() rgb modes
	d.laboff = 0;//hard coded, copied from callback() rgb modes
	//strcat(d.filename, "/media/sf_enc/032t.acv");//hard coded, file path for test
	//vdub end

	GradCurveData * data = new GradCurveData(d);

	vsapi->createFilter(in, out, "GradCurve", GradCurveInit, GradCurveGetFrame, GradCurveFree, fmParallelRequests, 0, data, core);
}

//////////////////////////////////////////
// Init

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin) {
	configFunc("com.holywu.gradcurve", "grad", "Adjustment of contrast, brightness, gamma and a wide range of color manipulations through gradation curves is possible.", VAPOURSYNTH_API_VERSION, 1, plugin);
	registerFunc("Curve",
		"clip:clip;"
		"var:data:opt;"
		"uvar:float:opt;"
		"hcorr:float:opt;"
		"vcorr:float:opt;"
		"seed:int:opt;"
		"constant:int:opt;",
		GradCurveCreate, nullptr, plugin);
}
