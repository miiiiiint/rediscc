#include <iostream>
#include <vector>
using namespace std;

// 函数：打印一个整数数组
void printArray( const vector< int >& arr ) {
  for ( int i = 0; i < arr.size(); i++ ) {
    cout << arr[ i ] << " ";
  }
  cout << endl;
}

// 函数：计算数组中所有元素的和
int sumArray( const vector< int >& arr ) {
  int sum = 0;
  for ( int i = 0; i < arr.size(); i++ ) {
    sum += arr[ i ];
  }
  return sum;
}

int main() {
  // 创建一个整数数组并初始化
  vector< int > numbers = { 1, 2, 3, 4, 5 };

  // 打印数组
  cout << "Array: ";
  printArray( numbers );

  // 计算数组的和并输出
  int total = sumArray( numbers );
  cout << "Sum of array elements: " << total << endl;

  return 0;
}
