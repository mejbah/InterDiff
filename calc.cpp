#include<iostream>
#include<fstream>
#include<sstream>
#include<math.h>
#include<string>
#include<assert.h>
using namespace std;
#define BUF_LEN 100

int total_threads;
fstream conf_file;
int main( int argc, char **args ) {
  if(argc != 4 ) {
    cout<<"Usage: rundiff data_dir execno conf_file" << endl;
    return 1;
  }
  conf_file.open(args[3], fstream::in);
  if( !conf_file.is_open()) {
    cout << args[3] <<" conf_file file not found" << endl;
    return 1;
  }
  string str;
  getline(conf_file, str);
  stringstream ss(str);
  ss >> total_threads;
  cout << "total threads " << total_threads << endl;

  int no_exec; 
  string exec_argv(args[2]);
  stringstream ss1(exec_argv);
  ss1 >> no_exec;
  cout << "total excutions " << no_exec << endl;
 // int no_exec2; 
 // string exec2_argv(args[3]);
//  stringstream ss2(exec2_argv);
 // ss2 >> no_exec2;
   
  fstream src_file, dest_file; 
  char src_file_name[BUF_LEN];
  char dest_file_name[BUF_LEN];

  for( int i=1; i< no_exec; i++){
    for( int j=i+1; j<=no_exec; j++) {
      
      sprintf( src_file_name, "%s/counter.report.%d", args[1], i);
      sprintf( dest_file_name, "%s/counter.report.%d", args[1], j);

 
      src_file.open( src_file_name, fstream::in );
      if( !src_file.is_open() ){
        cout<< src_file_name  << " not found" << endl;
        return 1;
      } 
      dest_file.open( dest_file_name, fstream::in);
      if( !dest_file.is_open() ){
        cout<< dest_file_name  << " not found" << endl;
        return 1;
      }   
      int dist_square_sum = 0;
      int active_pairs = 0;
      int src_total=0;
      int dest_total=0;
      for( int k=0; k< total_threads*total_threads; k++ ){
        string src_str, dest_str;
        getline(src_file, src_str);
        getline(dest_file, dest_str);
        stringstream src_ss(src_str);
        stringstream dest_ss(dest_str);
        int src_val, dest_val;
        src_ss>> src_val;
        dest_ss>>dest_val;
        src_total +=src_val;
        dest_total +=dest_val;
        if(src_val == 0 && dest_val == 0) {
      //    assert( dest_val == 0 );
          continue;
        }
	
        else {
          active_pairs++;
       //   cout << (src_val - dest_val) << endl;
          dist_square_sum += ((src_val - dest_val)*(src_val - dest_val));
        }
      }
      assert(src_total == dest_total);
      //double result =  (sqrt(dist_square_sum))/(double)active_pairs;
      double result =  ((sqrt(dist_square_sum))/(double)src_total) * 100;
      cout << "VAREXEC ( " << i << ","<< j<< " ) :: " << result << "%" << endl; 
      	
      //cout << "-- Active thread pairs " << active_pairs << " total shared load " << src_total << endl; 


      src_file.close();
      dest_file.close();
    }
    
  }  
}
