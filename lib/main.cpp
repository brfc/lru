#include <iostream>
#include <fstream>
#include <chrono>
#include <unordered_map>
#include <memory>
#include "libecho.hpp"

// Requirements
// Cache system with LRU discard policy
// the cache has a size N of chunks (each chunk needs to have the same size)
// key: begin x end
// name: a file name


// number of bytes in a chunk
const int CHUNK_SIZE = 128;

template<typename T, typename... Types>
T key(T d, Types... d2) 
{
  static_assert(
      (std::is_same<T,Types>::value && ...),
      "Key argunments must have the same type."
  );

  return (d ^ ... ^ d2);
}

/* 
 * Least recently used cache 
 *  _id : fd 
 * */
template <typename Functor, size_t N>
class LRU
{
  using FRT = typename std::result_of_t<Functor>;

private:
  
  std::string _id;
  size_t _MAX;

  struct Node
  {
    char _data[CHUNK_SIZE];
    size_t _csize;
  };

  typedef std::shared_ptr<Node> P_Node;
  /* cache data */
  std::unordered_map<FRT,P_Node> _cdata;
  
  /* cache LRU linear policy */
  size_t _size;

  /* O(1) LRU accesses */
  struct Item
  {
    std::chrono::time_point<std::chrono::system_clock> 
    _laccess = std::chrono::system_clock::now(); 
  
    P_Node _data;
  };
  std::list<Item> _times;

  /* O(1) index to list */
  typedef std::shared_ptr<Item> P_Item;
  std::unordered_map<FRT, P_Item> _tindex;

public:

};






void read_from_file(
      const std::string file_name,
      const int from_byte,
      const int to_byte)
{
  std::ifstream inputFile(file_name);
  
  inputFile.seekg(from_byte);
  

  int nreads = 0;
  int expected_reads = to_byte - from_byte;
  char chunk[CHUNK_SIZE];
  int read_calls;
  while(inputFile.read(chunk, CHUNK_SIZE)) 
  {
    read_calls ++;
    nreads +=inputFile.gcount();
    if (nreads >= expected_reads)
      break;
  }

  std::cout << "Bytes read: " << expected_reads \
    << std::endl;
  std::cout << "Read calls: " << read_calls \
    << std::endl;
  
}

int main()
{
  const std::string file_name = "file1";
  std::ifstream inputFile(file_name);

  std::string buffer;

  int i = 0;
  char chunk[CHUNK_SIZE];
  std::string data; 
  // this file was read once
  while(inputFile.read(chunk, CHUNK_SIZE))
  {
    i++;
    data.append(chunk, inputFile.gcount());
  }
 
  // an independent process is going to read
  // from byte to byte
  // call is istrumented to measure times
  perf({
  read_from_file(file_name, 10000000, 100000000);
  })
  return 0;
}
