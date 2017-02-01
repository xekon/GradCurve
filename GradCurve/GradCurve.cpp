// VapourSynth port by Xekon
//
// Copyright (c) 2008 Alexander Nagiller
//      http://members.chello.at/nagiller/vdub/readme.html
//      http://forum.doom9.org/showthread.php?t=133191
//  Speed optimizations for HSV and CMYK by Achim Stahlberger.
//  modified by Xekon:
//      http://forum.doom9.org/showthread.php?t=174096
/////////////////////////////////////////////////////////////////////////////
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
// Change Log
//
// Date         Version  Developer           Changes
//
// 21 Mar 2008  1.4.5.0  Alexander Nagiller  Final Release for VirtualDub.
//						 http://members.chello.at/nagiller/vdub/history.html
//
// 24 Dec 2016  1.5.0.0  Xekon               First release, Port for VapourSynth
// 01 Feb 2017  2.0.0.0  Xekon               Added remaining functionality
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <algorithm>
#include <cmath>
#include <VapourSynth.h>
#include <VSHelper.h>
#include <string>

int64_t *rgblab; //LUT Lab
int64_t *labrgb; //LUT Lab

struct GradCurveData {
	VSNodeRef * node;
	const VSVideoInfo * vi;
	int rvalue[3][256];
	int gvalue[3][256];
	int bvalue[256];
	int ovalue[5][256];
	std::string filename;// curve file name
	int filter;//curve file type: 1=amp (default), 2=ACV, 3=csv, 4=crv, 5=map, 6=amp Smartvurve hsv
	int proces;//process mode: 0=RGB only, 1=RGB + R/G/B (default), 2=RGB weighted, 3=RGB weighted + R/G/B, 4=no processing, 5=YUV, 6=CMYK, 7=HSV, 8=LAB
	int channel_mode;
	int drwmode[5];
	int drwpoint[5][16][2];
	int poic[5];
	char gamma[10];
	bool Labprecalc;
};

void PreCalcLut()
{
	int64_t count;
	int64_t x;
	int64_t y;
	int64_t z;
	int64_t rr;
	int64_t gg;
	int64_t bb;
	int64_t r1;
	int64_t g1;
	int64_t b1;
	int r;
	int g;
	int b;
	rgblab = new int64_t[16777216];
	labrgb = new int64_t[16777216];

	count = 0;
	for (r = 0; r<256; r++) {
		for (g = 0; g<256; g++) {
			for (b = 0; b<256; b++) {
				if (r > 10) { rr = int64_t(pow(((r << 4) + 224.4), (2.4))); }
				else { rr = int64_t((r << 4)*9987.749); }
				if (g > 10) { gg = int64_t(pow(((g << 4) + 224.4), (2.4))); }
				else { gg = int64_t((g << 4)*9987.749); }
				if (b > 10) { bb = int64_t(pow(((b << 4) + 224.4), (2.4))); }
				else { bb = int64_t((b << 4)*9987.749); }
				x = int64_t((rr + 6.38287545) / 12.7657509 + (gg + 7.36187255) / 14.7237451 + (bb + 14.58712555) / 29.1742511);
				y = int64_t((rr + 12.37891725) / 24.7578345 + (gg + 3.68093628) / 7.36187256 + (bb + 36.4678139) / 72.9356278);
				z = int64_t((rr + 136.1678335) / 272.335667 + (gg + 22.0856177) / 44.1712354 + (bb + 2.76970661) / 5.53941322);
				//XYZ to Lab
				if (x>841776) { rr = int64_t(pow((x), (0.33333333333333333333333333333333))*21.9122842); }
				else { rr = int64_t((x + 610.28989295) / 1220.5797859 + 1379.3103448275862068965517241379); }
				if (y>885644) { gg = int64_t(pow((y), (0.33333333333333333333333333333333))*21.5443498); }
				else { gg = int64_t((y + 642.0927467) / 1284.1854934 + 1379.3103448275862068965517241379); }
				if (z>964440) { bb = int64_t(pow((z), (0.33333333333333333333333333333333))*20.9408726); }
				else { bb = int64_t((z + 699.1298454) / 1398.2596908 + 1379.3103448275862068965517241379); }
				x = int64_t(((gg + 16.90331) / 33.806620) - 40.8);
				y = int64_t(((rr - gg + 7.23208898) / 14.46417796) + 119.167434);
				z = int64_t(((gg - bb + 19.837527645) / 39.67505529) + 135.936123);
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
				rr = int64_t(y*21.392519204 - 2549.29163142 + gg);
				bb = int64_t(gg - z*58.67940678 + 7976.6510628);
				if (gg>3060) { g1 = int64_t(gg*gg / 32352.25239*gg); }
				else { g1 = int64_t(x*43413.9788); }
				if (rr>3060) { r1 = int64_t(rr*rr / 34038.16258*rr); }
				else { r1 = int64_t(rr*825.27369 - 1683558); }
				if (bb>3060) { b1 = int64_t(bb*bb / 29712.85911*bb); }
				else { b1 = int64_t(bb*945.40885 - 1928634); }
				//XYZ to RGB
				rr = int64_t(r1*16.20355 + g1*-7.6863 + b1*-2.492855);
				gg = int64_t(r1*-4.84629 + g1*9.37995 + b1*0.2077785);
				bb = int64_t(r1*0.278176 + g1*-1.01998 + b1*5.28535);
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
			d->rvalue[0][i] = (d->ovalue[0][i]);
			d->rvalue[2][i] = (d->ovalue[0][i] - i);
			d->gvalue[0][i] = (d->ovalue[0][i]);
			d->gvalue[2][i] = (d->ovalue[0][i] - i);
			d->bvalue[i] = d->ovalue[0][i] - i;
		}
		break;
	case 1:
		for (i = 0; i<256; i++) { d->rvalue[1][i] = (d->ovalue[1][i]); }
		break;
	case 2:
		for (i = 0; i<256; i++) { d->gvalue[1][i] = (d->ovalue[2][i]); }
		break;
	}
}

void ImportCurve(GradCurveData * d)
{
	FILE *pFile;
	int i;
	int j;
	int stor[1280];
	int temp[1280];
	int64_t lSize = 0;
	int beg;
	int cv;
	int count = 0;
	int noocur = 0;
	int curpos;
	int cordpos;
	int curposnext = 0;
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
		pFile = fopen(d->filename.c_str(), "rb");
		if (pFile == NULL) {
			printf("GradCurve: Error opening file.\n");
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
		pFile = fopen(d->filename.c_str(), "r");
		if (pFile == NULL) {
			printf("GradCurve: Error opening file.\n");
		}
		else
		{
			fseek(pFile, 0, SEEK_END);
			lSize = ftell(pFile);
			rewind(pFile);
			for (i = 0; (i < 1280) && (feof(pFile) == 0); i++)
			{
				if (fscanf(pFile, "%d", &stor[i]) != 1) {
					printf("GradCurve: Failed to read integer from csv curve file.\n");
				}
			}
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
		pFile = fopen(d->filename.c_str(), "rb");
		if (pFile == NULL) {
			printf("GradCurve: Error opening file.\n");
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
				if (i == beg + 2 && d->drwmode[curpos] == 3) { gma = gma + (cv); }
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
		nrf = true;
	}
	else if (d->filter == 6)	// *.amp Smartvurve hsv
	{
		pFile = fopen(d->filename.c_str(), "rb");
		if (pFile == NULL) {
			printf("GradCurve: Error opening file.\n");
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
		pFile = fopen(d->filename.c_str(), "rb"); // *.amp
		if (pFile == NULL) {
			printf("GradCurve: Error opening file.\n");
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
	if (nrf == false) { //fill curves for non coordinates file types amp/csv/amp hsv
		if (lSize > 768) {
			for (i = 0; i < 256; i++) {
				d->ovalue[0][i] = stor[i];
				d->rvalue[0][i] = (d->ovalue[0][i]);
				d->rvalue[2][i] = (d->ovalue[0][i] - i);
				d->gvalue[0][i] = (d->ovalue[0][i]);
				d->gvalue[2][i] = (d->ovalue[0][i] - i);
				d->bvalue[i] = d->ovalue[0][i] - i;
			}
			for (i = 256; i < 512; i++) {
				d->ovalue[1][(i - 256)] = stor[i];
				d->rvalue[1][(i - 256)] = (d->ovalue[1][(i - 256)]);
			}
			for (i = 512; i < 768; i++) {
				d->ovalue[2][(i - 512)] = stor[i];
				d->gvalue[1][(i - 512)] = (d->ovalue[2][(i - 512)]);
			}
			for (i = 768; i < 1024; i++) { d->ovalue[3][(i - 768)] = stor[i]; }
			for (i = 1024; i < 1280; i++) { d->ovalue[4][(i - 1024)] = stor[i]; }
		}
		if (lSize < 769 && lSize > 256) {
			for (i = 0; i < 256; i++) {
				d->ovalue[1][i] = stor[i];
				d->rvalue[1][i] = (d->ovalue[1][i]);
			}
			for (i = 256; i < 512; i++) {
				d->ovalue[2][(i - 256)] = stor[i];
				d->gvalue[1][(i - 256)] = (d->ovalue[2][(i - 256)]);
			}
			for (i = 512; i < 768; i++) { d->ovalue[3][(i - 512)] = stor[i]; }
		}
		if (lSize < 257 && lSize > 0) {
			for (i = 0; i < 256; i++) {
				d->ovalue[0][i] = stor[i];
				d->rvalue[0][i] = (d->ovalue[0][i]);
				d->rvalue[2][i] = (d->ovalue[0][i] - i);
				d->gvalue[0][i] = (d->ovalue[0][i]);
				d->gvalue[2][i] = (d->ovalue[0][i] - i);
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

		const int width = vsapi->getFrameWidth(src, 0);
		const int height = vsapi->getFrameHeight(src, 0);
		const int srcStride = vsapi->getStride(src, 0);
		const int dstStride = vsapi->getStride(dst, 0);
		const unsigned char * srcpR = reinterpret_cast<const unsigned char *>(vsapi->getReadPtr(src, 0));
		const unsigned char * srcpG = reinterpret_cast<const unsigned char *>(vsapi->getReadPtr(src, 1));
		const unsigned char * srcpB = reinterpret_cast<const unsigned char *>(vsapi->getReadPtr(src, 2));
		unsigned char * VS_RESTRICT dstpR = reinterpret_cast<unsigned char *>(vsapi->getWritePtr(dst, 0));
		unsigned char * VS_RESTRICT dstpG = reinterpret_cast<unsigned char *>(vsapi->getWritePtr(dst, 1));
		unsigned char * VS_RESTRICT dstpB = reinterpret_cast<unsigned char *>(vsapi->getWritePtr(dst, 2));
		int64_t r, rr, b, g, bb, gg;
		switch (d->proces)
		{
		case 0://RGB (default)
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					r = static_cast<int>(srcpR[x]);
					g = static_cast<int>(srcpG[x]);
					b = static_cast<int>(srcpB[x]);
					dstpR[x] = static_cast<uint8_t>(d->rvalue[0][r]);
					dstpG[x] = static_cast<uint8_t>(d->gvalue[0][g]);
					dstpB[x] = static_cast<uint8_t>(d->ovalue[0][b]);
				}
				srcpR += srcStride;
				srcpG += srcStride;
				srcpB += srcStride;
				dstpR += dstStride;
				dstpG += dstStride;
				dstpB += dstStride;
			}
			break;
		case 1://RGB + R/G/B
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					r = static_cast<int>(srcpR[x]);
					g = static_cast<int>(srcpG[x]);
					b = static_cast<int>(srcpB[x]);
					rr = d->rvalue[1][r];
					gg = d->gvalue[1][g];
					bb = d->ovalue[3][b];
					dstpR[x] = static_cast<uint8_t>(d->rvalue[0][rr]);
					dstpG[x] = static_cast<uint8_t>(d->gvalue[0][gg]);
					dstpB[x] = static_cast<uint8_t>(d->ovalue[0][bb]);
				}
				srcpR += srcStride;
				srcpG += srcStride;
				srcpB += srcStride;
				dstpR += dstStride;
				dstpG += dstStride;
				dstpB += dstStride;
			}
			break;
		case 2://RGB weighted
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					r = static_cast<int>(srcpR[x] << 16);
					g = static_cast<int>(srcpG[x] << 8);
					b = static_cast<int>(srcpB[x]);
					int64_t bw = int((77 * (r >> 16) + 150 * (g >> 8) + 29 * b) >> 8);
					r = r + d->rvalue[2][bw];
					if (r<65536) r = 0; else if (r>16711680) r = 16711680;
					g = g + d->gvalue[2][bw];
					if (g<256) g = 0; else if (g>65280) g = 65280;
					b = b + d->bvalue[bw];
					if (b<0) b = 0; else if (b>255) b = 255;
					dstpR[x] = static_cast<uint8_t>((r & 0xFF0000) >> 16);
					dstpG[x] = static_cast<uint8_t>((g & 0x00FF00) >> 8);
					dstpB[x] = static_cast<uint8_t>((b & 0x0000FF));
				}
				srcpR += srcStride;
				srcpG += srcStride;
				srcpB += srcStride;
				dstpR += dstStride;
				dstpG += dstStride;
				dstpB += dstStride;
			}
			break;
		case 3://RGB weighted + R/G/B
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					r = static_cast<int>(srcpR[x]);
					g = static_cast<int>(srcpG[x]);
					b = static_cast<int>(srcpB[x]);
					rr = d->rvalue[1][r] << 16;
					gg = d->gvalue[1][g] << 8;
					bb = d->ovalue[3][b];
					int64_t bw = int((77 * (rr >> 16) + 150 * (gg >> 8) + 29 * bb) >> 8);
					rr = rr + d->rvalue[2][bw];
					if (rr<65536) rr = 0; else if (rr>16711680) rr = 16711680;
					gg = gg + d->gvalue[2][bw];
					if (gg<256) gg = 0; else if (gg>65280) gg = 65280;
					bb = bb + d->bvalue[bw];
					if (bb<0) bb = 0; else if (bb>255) bb = 255;
					dstpR[x] = static_cast<uint8_t>((rr & 0xFF0000) >> 16);
					dstpG[x] = static_cast<uint8_t>((gg & 0x00FF00) >> 8);
					dstpB[x] = static_cast<uint8_t>((bb & 0x0000FF));
				}
				srcpR += srcStride;
				srcpG += srcStride;
				srcpB += srcStride;
				dstpR += dstStride;
				dstpG += dstStride;
				dstpB += dstStride;
			}
			break;
		case 4://No processing
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					r = static_cast<int>(srcpR[x]);
					g = static_cast<int>(srcpG[x]);
					b = static_cast<int>(srcpB[x]);
					dstpR[x] = static_cast<uint8_t>(r);
					dstpG[x] = static_cast<uint8_t>(g);
					dstpB[x] = static_cast<uint8_t>(b);
				}
				srcpR += srcStride;
				srcpG += srcStride;
				srcpB += srcStride;
				dstpR += dstStride;
				dstpG += dstStride;
				dstpB += dstStride;
			}
			break;
		case 5://YUV
			int64_t s, t, u;
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					r = static_cast<int>(srcpR[x]);
					g = static_cast<int>(srcpG[x]);
					b = static_cast<int>(srcpB[x]);
					//RGB to YUV (s=Y t=U u=V)
					s = (32768 + 19595 * r + 38470 * g + 7471 * b) >> 16; //correct rounding +32768 
					t = (8421375 - 11058 * r - 21710 * g + 32768 * b) >> 16; //correct rounding +32768
					u = (8421375 + 32768 * r - 27439 * g - 5329 * b) >> 16; //correct rounding +32768																			 
					s = (d->ovalue[1][s]) << 16;// Applting the curves
					t = (d->ovalue[2][t]) - 128;
					u = (d->ovalue[3][u]) - 128;
					// YUV to RGB
					rr = (32768 + s + 91881 * u); //correct rounding +32768
					if (rr<0) { r = 0; }
					else if (rr>16711680) { r = 16711680; }
					else { r = (rr & 0xFF0000); }
					gg = (32768 + s - 22553 * t - 46802 * u); //correct rounding +32768
					if (gg<0) { g = 0; }
					else if (gg>16711680) { g = 65280; }
					else { g = (gg & 0xFF0000) >> 8; }
					bb = (32768 + s + 116130 * t); //correct rounding +32768
					if (bb<0) { b = 0; }
					else if (bb>16711680) { b = 255; }
					else { b = bb >> 16; }
					dstpR[x] = static_cast<uint8_t>((r & 0xFF0000) >> 16);
					dstpG[x] = static_cast<uint8_t>((g & 0x00FF00) >> 8);
					dstpB[x] = static_cast<uint8_t>((b & 0x0000FF));
				}
				srcpR += srcStride;
				srcpG += srcStride;
				srcpB += srcStride;
				dstpR += dstStride;
				dstpG += dstStride;
				dstpB += dstStride;
			}
			break;
		case 6://CMYK
			int64_t e, f, v, z, div, divh;
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					r = static_cast<int>(srcpR[x]);
					g = static_cast<int>(srcpG[x]);
					b = static_cast<int>(srcpB[x]);
					if (r >= g && r >= b) {/* r is Maximum */
						v = 255 - r;
						div = r + 1;
						divh = div >> 1;
						e = 0;
						f = (((r - g) << 8) + divh) / div;//correct rounding  ff+(div>>1)
						z = (((r - b) << 8) + divh) / div;//correct rounding  zz+(div>>1)
					}
					else if (g >= b) {/* g is maximum */
						v = 255 - g;
						div = g + 1;
						divh = div >> 1;
						e = (((g - r) << 8) + divh) / div;//correct rounding  ee+(div>>1)
						f = 0;
						z = (((g - b) << 8) + divh) / div;//correct rounding  zz+(div>>1)
					}
					else {/* b is maximum */
						v = 255 - b;
						div = b + 1;
						divh = div >> 1;
						e = (((b - r) << 8) + divh) / div;//correct rounding  ee+(div>>1)
						f = (((b - g) << 8) + divh) / div;//correct rounding  ff+(div>>1)
						z = 0;
					}
					// Applying the curves
					e = d->ovalue[1][e];
					f = d->ovalue[2][f];
					z = d->ovalue[3][z];
					v = d->ovalue[4][v];
					// CMYK to RGB
					r = 255 - ((((e*(256 - v)) + 128) >> 8) + v); //correct rounding rr+128;
					if (r<0) r = 0;
					g = 255 - ((((f*(256 - v)) + 128) >> 8) + v); //correct rounding gg+128;
					if (g<0) g = 0;
					b = 255 - ((((z*(256 - v)) + 128) >> 8) + v); //correct rounding bb+128;
					if (b<0) b = 0;
					dstpR[x] = static_cast<uint8_t>(r);
					dstpG[x] = static_cast<uint8_t>(g);
					dstpB[x] = static_cast<uint8_t>(b);
				}
				srcpR += srcStride;
				srcpG += srcStride;
				srcpB += srcStride;
				dstpR += dstStride;
				dstpG += dstStride;
				dstpB += dstStride;
			}
			break;
		case 7://HSV
			int64_t l, m, n, cmin, cdelta, cdeltah, chi, ch;
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					r = static_cast<int>(srcpR[x]);
					g = static_cast<int>(srcpG[x]);
					b = static_cast<int>(srcpB[x]);
					//RGB to HSV (l=H m=S n=V)
					cmin = (std::min)(r, g);
					cmin = (std::min)(b, cmin);
					n = (std::max)(r, g);
					n = (std::max)(b, n);
					cdelta = n - cmin;
					if (cdelta != 0)
					{
						m = (cdelta * 255) / n;
						cdelta = (cdelta * 6);
						cdeltah = cdelta >> 1;
						if (r == n) { l = (((g - b) << 16) + cdeltah) / cdelta; }
						else if (g == n) { l = 21845 + ((((b - r) << 16) + cdeltah) / cdelta); }
						else { l = 43689 + ((((r - g) << 16) + cdeltah) / cdelta); }
						if (l<0) { l = (l + 65577) >> 8; }
						else { l = (l + 128) >> 8; }
					}
					else
					{
						m = 0;
						l = 0;
					}
					// Applying the curves
					l = d->ovalue[1][l];
					m = d->ovalue[2][m];
					n = d->ovalue[3][n];
					// HSV to RGB
					if (m == 0)
					{
						r = n;
						g = n;
						b = n;
					}
					else
					{
						chi = ((l * 6) & 0xFF00);
						ch = (l * 6 - chi);;
						switch (chi)
						{
						case 0:
							r = n;
							g = (n*(65263 - (m*(256 - ch))) + 65531) >> 16;
							b = (n*(255 - m) + 94) >> 8;
							break;
						case 256:
							r = (n*(65263 - m*ch) + 65528) >> 16;
							g = n;
							b = (n*(255 - m) + 89) >> 8;
							break;
						case 512:
							r = (n*(255 - m) + 89) >> 8;
							g = n;
							b = (n*(65267 - (m*(256 - ch))) + 65529) >> 16;
							break;
						case 768:
							r = (n*(255 - m) + 89) >> 8;
							g = (n*(65267 - m*ch) + 65529) >> 16;
							b = n;
							break;
						case 1024:
							r = (n*(65263 - (m*(256 - ch))) + 65528) >> 16;
							g = (n*(255 - m) + 89) >> 8;
							b = n;
							break;
						default:
							r = n;
							g = (n*(255 - m) + 89) >> 8;
							b = (n*(65309 - m*(ch + 1)) + 27) >> 16;
							break;
						}
					}
					dstpR[x] = static_cast<uint8_t>(r);
					dstpG[x] = static_cast<uint8_t>(g);
					dstpB[x] = static_cast<uint8_t>(b);
				}
				srcpR += srcStride;
				srcpG += srcStride;
				srcpB += srcStride;
				dstpR += dstStride;
				dstpG += dstStride;
				dstpB += dstStride;
			}
			break;
		case 8: //LAB
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					r = static_cast<int>(srcpR[x] << 16);
					g = static_cast<int>(srcpG[x] << 8);
					b = static_cast<int>(srcpB[x]);
					int64_t old_pixel = r + g + b;
					int64_t lab = rgblab[(old_pixel & 0xFFFFFF)];
					rr = (lab & 0xFF0000) >> 16;
					gg = (lab & 0x00FF00) >> 8;
					bb = (lab & 0x0000FF);
					// Applying the curves
					x = d->ovalue[1][rr];
					y = d->ovalue[2][gg];
					z = d->ovalue[3][bb];
					//Lab to XYZ				
					int64_t new_pixel = labrgb[((x << 16) + (y << 8) + z)];
					dstpR[x] = static_cast<uint8_t>((new_pixel & 0xFF0000) >> 16);
					dstpG[x] = static_cast<uint8_t>((new_pixel & 0x00FF00) >> 8);
					dstpB[x] = static_cast<uint8_t>((new_pixel & 0x0000FF));
				}
				srcpR += srcStride;
				srcpG += srcStride;
				srcpB += srcStride;
				dstpR += dstStride;
				dstpG += dstStride;
				dstpB += dstStride;
			}
			break;
		}
		vsapi->freeFrame(src);
		return dst;
	}
	return nullptr;
}

static void VS_CC GradCurveFree(void *instanceData, VSCore *core, const VSAPI *vsapi) {
	GradCurveData * d = static_cast<GradCurveData *>(instanceData);
	vsapi->freeNode(d->node);
	delete d;
	delete[] rgblab; //LUT Lab
	delete[] labrgb; //LUT Lab
}

static void VS_CC GradCurveCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi) {
	GradCurveData d;
	int err;
	d.filename = static_cast<std::string>(vsapi->propGetData(in, "fname", 0, &err));//curve file path
	d.filter = int64ToIntS(vsapi->propGetInt(in, "ftype", 0, &err));; //curve file type eg: amp
	if (d.filter == 0) {//filetype not provided, try to decipher from extension.
		std::string mext = d.filename.substr(d.filename.size() - 4);
		std::transform(mext.begin(), mext.end(), mext.begin(), ::tolower);
		if (mext == ".amp") { d.filter = 1; }
		else if (mext == ".acv") { d.filter = 2; }
		else if (mext == ".csv") { d.filter = 3; }
		else if (mext == ".crv") { d.filter = 4; }
		else if (mext == ".map") { d.filter = 5; }
		else if (mext == ".hsv") { d.filter = 6; }
		else { vsapi->setError(out, "GradCurve: not a valid curve file, use: amp, acv, csv, crv, map, hsv"); }
	}
	d.proces = int64ToIntS(vsapi->propGetInt(in, "pmode", 0, &err));; //process mode eg: RGB + R/G/B
	d.channel_mode = 0;//0=ALL, 1=R, 2=G, This should probably stay 0 since GradCurve only applies curves and does not manipulate them
	d.node = vsapi->propGetNode(in, "clip", 0, nullptr);
	d.vi = vsapi->getVideoInfo(d.node);

	if (!isConstantFormat(d.vi) || d.vi->format->sampleType != stInteger || d.vi->format->bitsPerSample != 8) {
		vsapi->setError(out, "GradCurve: only constant format 8-bit integer input supported: src = core.resize.Bicubic(clip=src, format=vs.RGB24)");
		vsapi->freeNode(d.node);
		return;
	}
	d.Labprecalc = 0;
	for (int i = 0; i<5; i++) {
		d.drwmode[i] = 2;
		d.poic[i] = 2;
		d.drwpoint[i][0][0] = 0;
		d.drwpoint[i][0][1] = 0;
		d.drwpoint[i][1][0] = 255;
		d.drwpoint[i][1][1] = 255;
	}
	snprintf(d.gamma, 10, "%.3lf", 1.000);
	for (int i = 0; i<256; i++) {
		d.ovalue[0][i] = i;
		d.rvalue[0][i] = (d.ovalue[0][i]);
		d.rvalue[2][i] = (d.ovalue[0][i] - i);
		d.gvalue[0][i] = (d.ovalue[0][i]);
		d.gvalue[2][i] = (d.ovalue[0][i] - i);
		d.bvalue[i] = (d.ovalue[0][i] - i);
		d.ovalue[1][i] = i;
		d.rvalue[1][i] = (d.ovalue[1][i]);
		d.ovalue[2][i] = i;
		d.gvalue[1][i] = (d.ovalue[2][i]);
		d.ovalue[3][i] = i;
		d.ovalue[4][i] = i;
	}
	ImportCurve(&d);
	if (d.Labprecalc == 0 && d.proces == 8) {	// build up the LUT for the Lab process	if it is not precalculated already
		PreCalcLut();
		d.Labprecalc = 1;
	}
	GradCurveData * data = new GradCurveData(d);
	vsapi->createFilter(in, out, "GradCurve", GradCurveInit, GradCurveGetFrame, GradCurveFree, fmParallel, 0, data, core);
}

//////////////////////////////////////////
// Init
VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin) {
	configFunc("com.xekon.gradcurve", "grad", "Adjustment of contrast, brightness, gamma and a wide range of color manipulations through gradation curves is possible.", VAPOURSYNTH_API_VERSION, 1, plugin);
	registerFunc("Curve",
		"clip:clip;"
		"fname:data:opt;"//curve file name
		"ftype:int:opt;" //curve file type: 1=amp (default), 2=ACV, 3=csv, 4=crv, 5=map, 6=amp Smartvurve hsv
		"pmode:int:opt;",//process mode: 0=RGB (default), 1=RGB + R/G/B, 2=RGB weighted, 3=RGB weighted + R/G/B, 4=none, 5=YUV, 6=CMYK, 7=HSV, 8=LAB
		GradCurveCreate, nullptr, plugin);
}
