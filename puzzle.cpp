/*
元画像をN*Nに分割する。分割した画像（以下ピース）は、pairで1~N*Nまでの番号と結びつけ、配列に入れる。
そのピースを順番をバラバラにしてくっつけ直してパズルを作る。
パズルを敵用と自分用を用意する。
↓ここから並列処理。
①敵のパズル
ピースの番号によってそのピースの正しい位置がわかるため、左上からピースを元の位置に戻していく。
②自分のパズル
右クリックしたピースと左クリックしたピースの位置を入れ替える。ピースの入れ替えは、配列内での順番を入れ替えくっつけ直す。
*/

#include <pthread.h>
#include <unistd.h>
#include <opencv/cv.h>
#include <opencv/cvaux.h>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <algorithm>
#include <omp.h>
using namespace std;
using namespace cv;
#define font FONT_HERSHEY_DUPLEX
#define N 4        //N×Nのパズル
#define P pair<Mat,int>
int katimake;

bool kinkyuu=false;

Mat src;
int ix1=-1,iy1,ix2=-1,iy2;
vector< vector<P> > a(N,vector<P>(N));
vector< vector<P> > b(N,vector<P>(N));

Mat renketu(vector< vector<P> >); //分割した画像をつなげる関数
void on_mouse(int event,int x,int y,int flags,void *param);


int main(int argc,char** argv){
  srand((unsigned)time(NULL));
  int i,j;
  if(argc>=2){
    src=imread(argv[1],1);
  }else{
    cout<<"画像を指定してください"<<endl;
  }
  cout<<"見本です。Enterをおしてください"<<endl;
  unsigned char c;
  namedWindow("mihon", CV_WINDOW_AUTOSIZE|CV_WINDOW_FREERATIO);
  imshow("mihon",src);
  while(1){
    c=cvWaitKey(2);
    if(c=='\x0a'){
      break;
    }
  }
  destroyWindow("mihon");
  
  
  // N*N分割するための点を定義
  vector< vector<Point> > p(N,vector<Point>(N));
  for(i=0;i<N;i++){
    for(j=0;j<N;j++){
      p[i][j]={src.cols*j/N, src.rows*i/N};
    }
  }
  
  // N*N分割し、N*Nの二次元配列に分割された画像を入れる。
  for(i=0;i<N;i++){
    j=0;
    for (auto itr = p[i].begin(); itr != p[i].end(); ++itr) {
      // 関心領域を元の画像から矩形で切り出す
      Mat roi = src(Rect(itr->x, itr->y, src.cols / N, src.rows / N));
      a[i][j]=make_pair(roi,N*i+j);
      j++;
    }
  }

  //分割された画像をランダムに並び替える。
  for(i=0;i<10000;i++){
    int i1,i2,j1,j2;
    i1=rand()%N;
    i2=rand()%N;
    j1=rand()%N;
    j2=rand()%N;
    swap(a[i1][j1],a[i2][j2]);
  }
  b=a;
  
  //画像を連結する。
  Mat my_puz,ene_puz;
  my_puz=ene_puz=renketu(a);

  //敵のパズルと自分のパズルを作成
  namedWindow("enemy_puzzle", CV_WINDOW_AUTOSIZE|CV_WINDOW_FREERATIO);
  imshow("enemy_puzzle",ene_puz);
  cvWaitKey(2);
  namedWindow("my_puzzle", CV_WINDOW_AUTOSIZE|CV_WINDOW_FREERATIO);
  setMouseCallback("my_puzzle",on_mouse,0);
  imshow("my_puzzle",my_puz);
  
  cout<<"Enterキーでゲーム開始です"<<endl;
  while(1){
    c=cvWaitKey(2);
    if(c=='\x0a'){
      break;
    }
  }

  //ここから並行処理

  #pragma omp parallel
  {
    #pragma omp sections
    {
      //敵のパズル
      #pragma omp section
      {
	int nokori=N*N; //入れ替わってないピースの個数
	int jx=0,jy=0;
	while(nokori>0){
	  //左上から正しいピースを埋めていく
	  if(kinkyuu){
	    break;
	  }
	  int jx2,jy2;
	  for(i=0;i<N;i++){
	    for(j=0;j<N;j++){
	      if(b[i][j].second==N*jy+jx){
		jy2=i; jx2=j;
	      }
	    }
	  }
	  swap(b[jy][jx],b[jy2][jx2]);
	  if(jx!=N-1){
	    jx++;
	  }else{
	    jx=0;
	    jy++;
	  }
	  nokori--;
	  ene_puz=renketu(b);
	  imshow("enemy_puzzle",ene_puz);
	  cvWaitKey(100);
	}
	cout<<"enemy:finished!!"<<endl;
	katimake=0;
	destroyWindow("enemy_puzzle");
      }
      //自分のパズル
      #pragma omp section
      {
	//詳しい動作はon_mouse内のコメント参照
        bool flag;
	while(1){
	  if(kinkyuu){
	    break;
	  }
	  my_puz=renketu(a);
	  imshow("my_puzzle",my_puz);
	  cvWaitKey(2);
	  //全部のピースが正しい位置に戻っていたらwhileを抜ける
	  flag=true;
	  for(i=0;i<N;i++){
	    for(j=0;j<N;j++){
	      if(a[i][j].second!=N*i+j){
		flag=false;
	      }
	    }
	  }
	  if(flag){
	    my_puz=renketu(a);
	    imshow("my_puzzle",my_puz);
	    cout<<"you:finished!!"<<endl;
	    katimake=1;
	    break;
	  }
	}
      }
    }
  }
  if(kinkyuu){
    cout<<"中止"<<endl;
  }else if(katimake){
    cout<<"you lose..."<<endl;
  }else{
    cout<<"you win!!"<<endl;
  }
  imshow("my_puzzle",my_puz);
  cout<<"Enterキーでゲーム終了です"<<endl;
  while(1){
    c=cvWaitKey(2);
    if(c=='\x0a'){
      break;
    }
  }
}


Mat renketu(vector< vector<P> > a){
  int i,j;
  Mat my_puz(Size(src.cols,src.rows), CV_8UC3);
  vector<Mat> c(N);
  for(i=0;i<N;i++){
    auto itr=a[i].begin();
    auto itr_end=a[i].end();
    Mat temp(Size(src.cols,src.rows/N),CV_8UC3);
    Rect roi_rect1;
    for(; itr!=itr_end; ++itr){
      roi_rect1.width=itr->first.cols;
      roi_rect1.height=itr->first.rows;
      Mat roi(temp,roi_rect1);
      itr->first.copyTo(roi);
      roi_rect1.x += itr->first.cols;
    }
    c[i]=temp;
  }
  auto itr=c.begin();
  auto itr_end=c.end();
  Rect roi_rect;
  for(; itr!=itr_end; ++itr){
    roi_rect.width=itr->cols;
    roi_rect.height=itr->rows;
    Mat roi(my_puz,roi_rect);
    itr->copyTo(roi);
    roi_rect.y += itr->rows;
  }
  
  return my_puz;
  
}



void on_mouse(int event,int x,int y,int flags,void *param){
  //左クリックでix1,iy1に、右クリックでix2,iy2に座標が入る。ホイールで緊急終了
  if(event==CV_EVENT_LBUTTONDOWN){
    ix1=x; iy1=y;
  }
  if(event==CV_EVENT_RBUTTONDOWN){
    ix2=x; iy2=y;
  }
  if(event==CV_EVENT_MBUTTONDOWN){
    kinkyuu=true;
  }
  int i1,j1,i2,j2;
  //ix1,ix2が正のとき、入れ替える。
  if(ix1>=0 && ix2>=0){ 
    for(j1=0;j1<N;j1++){
      if(ix1<src.cols*(j1+1)/N){
	break;
      }
    }
    for(j2=0;j2<N;j2++){
      if(ix2<src.cols*(j2+1)/N){
	break;
      }
    }
    for(i1=0;i1<N;i1++){
      if(iy1<src.rows*(i1+1)/N){
	break;
      }
    }
    for(i2=0;i2<N-1;i2++){
      if(iy2<src.rows*(i2+1)/N){
	break;
      }
    }
    swap(a[i1][j1],a[i2][j2]);
    ix1=-1; ix2=-1;
  }
}
  
