//Jacob Christiansen
#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include "Filter.h"

using namespace std;

#include "rdtsc.h"

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int main(int argc, char **argv){

  if(argc < 2){
    fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
  }

  //
  // Convert to C++ strings to simplify manipulation
  //
  string filtername = argv[1];

  //
  // remove any ".filter" in the filtername
  //
  string filterOutputName = filtername;
  string::size_type loc = filterOutputName.find(".filter");
  if (loc != string::npos){
    //
    // Remove the ".filter" name, which should occur on all the provided filters
    //
    filterOutputName = filtername.substr(0, loc);
  }

  Filter *filter = readFilter(filtername);

  double sum = 0.0;
  short samples = 0;

  for(short inNum = 2; inNum < argc; inNum++){
    string inputFilename = argv[inNum];
    string outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
    struct cs1300bmp *input = new struct cs1300bmp;
    struct cs1300bmp *output = new struct cs1300bmp;
    short ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);

    if ( ok ) {
      double sample = applyFilter(filter, input, output);
      sum += sample;
      samples++;
      cs1300bmp_writefile((char *) outputFilename.c_str(), output);
    }
    delete input;
    delete output;
  }
  fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);

}

struct Filter *
readFilter(string filename)
{
  ifstream input(filename.c_str());

  if ( ! input.bad() ) {
    short size = 0;
    input >> size;
    Filter *filter = new Filter(size);
    short div;
    input >> div;
    filter -> setDivisor(div);
    for (short i=0; i < size; i++) {
      for (short j=0; j < size; j++) {
	short value;
	input >> value;
	filter -> set(i,j,value);
      }
    }
    return filter;
  } else {
    cerr << "Bad input in readFilter:" << filename << endl;
    exit(-1);
  }
}


double
applyFilter(struct Filter *filter, cs1300bmp *input, cs1300bmp *output)
{

  long long cycStart, cycStop;

  cycStart = rdtscll();

  output -> width = input -> width;
  output -> height = input -> height;

//put variables outside of the for loops so we don't need to continually get their values from within
  short height = (input->height)-1;
  short width = (input->width)-1;
  // short size = filter->getSize();
  short divisor = filter->getDivisor();
  short *filterOut; //move the for loop that acceses the plane, outside. Speeds up access matrix process

  short get_ij[3][3];//filter->get(i, j)
  for(short i=0; i<3; i++){
    get_ij[i][0] = filter->get(i,0);
    get_ij[i][1] = filter->get(i,1);
    get_ij[i][2] = filter->get(i,2);
  }

  for(short plane = 0; plane < 3; plane++) {
    for(short row = 1; row < height; row++) {
      for(short col = 1; col < width; col++){

    	 //output -> color[plane][row][col] = 0;
        //let this be stored in to a \/ variable \/ b/c accessing takes longer
        short tempVar = 0;


      	// for (short j = 0; j < filter -> getSize(); j++){
      	//   for (short i = 0; i < filter -> getSize(); i++){	
      	//     output -> color[plane][row][col]
      	//       = output -> color[plane][row][col]
      	//       + (input -> color[plane][row + i - 1][col + j - 1] 
      	// 	 * filter -> get(i, j) );
      	//   }
      	// }
        //Loop Unrolling! (of loop above)
        filterOut = &input->color[plane][row-1][col-1];
        tempVar += *(filterOut++)*(get_ij[0][0]);
        tempVar += *(filterOut++)*(get_ij[0][1]);
        tempVar += *(filterOut++)*(get_ij[0][2]);

        filterOut = &input->color[plane][row][col-1];
        tempVar += *(filterOut++)*(get_ij[1][0]);
        tempVar += *(filterOut++)*(get_ij[1][1]);
        tempVar += *(filterOut++)*(get_ij[1][2]);

        filterOut = &input->color[plane][row+1][col-1];
        tempVar += *(filterOut++)*(get_ij[2][0]);
        tempVar += *(filterOut++)*(get_ij[2][1]);
        tempVar += *(filterOut++)*(get_ij[2][2]);

        tempVar /= divisor;
  	
      	// output -> color[plane][row][col] = 	
      	//   output -> color[plane][row][col] / filter -> getDivisor();

      	if(tempVar < 0 ){//replace output->color[plane][row][col] with tempVar
      	  tempVar = 0;
      	}
      	if(tempVar  > 255 ){ 
      	  tempVar = 255;
      	}
        output->color[plane][row][col] = tempVar;

      }
    }
  }

  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
	  diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}
