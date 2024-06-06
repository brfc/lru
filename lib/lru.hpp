#include <array>
#include <chrono>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>


const int CHUNK_SIZE = 128;
const int CACHE_SIZE = 50000000;

/* 
 * Least recently used cache : LRU policy
 *  _id : fd
 *  _Functor: key() function
 * */
template <size_t N>
class LRU
{
  using K = int;

public:
  int key(int d, int d2) 
  {
    return (d ^ d2);
  }
  
  std::string _id;
  size_t _MAX;

  struct Node
  {
    std::array<char, CHUNK_SIZE> data;
    size_t _csize;

    Node(std::array<char, CHUNK_SIZE>& d) : data(d), _csize(CHUNK_SIZE) {}
  };

  /* R1: 0 << _cdata.size() <= _size*/
  /* Change to pointer avoid copies*/
  size_t _size  = static_cast<size_t>(CACHE_SIZE);
  std::unordered_map<K,Node> _cdata;
  
  struct Item
  {
    std::chrono::time_point<std::chrono::system_clock> 
    _laccess;
  
    const K _k;

    Item(const K& k) : _laccess(std::chrono::system_clock::now()), _k(k) {}
  };
  std::list<Item> _times;

  using P_Item = typename std::list<Item>::iterator;
  std::unordered_map<K, P_Item> _tindex;

  /* lru policy */
  void _lru_policy() 
  {
    if (_times.size() > 0) {
       Item item = _times.front();
       _times.pop_front();
       _tindex.erase(item._k);
       _cdata.erase(item._k);
    }
  }

  void _update_key_last_access(const size_t k) 
  { 
    if (_tindex.find(k) != _tindex.end())
    {
      _times.erase(_tindex[k]); 
      _times.push_back(*_tindex[k]);
    }
    else 
    {
      _times.push_back(Item(k));
    }
    _tindex.emplace(k, std::prev(_times.end())); 
  };

  /* lookup chunk (bb-be) - O(1) */
  /* TODO: dont copy - use weak_ptr instead towards O(1) */
  std::optional<std::array<char,CHUNK_SIZE>> get(int bb, int be) 
  {
    auto k = key(bb,be);
    auto f = _cdata.find(k); 

    if(f!=_cdata.end()) {
      return std::optional<std::array<char,CHUNK_SIZE>>{f->second.data};
    }
    return std::nullopt;
  };

  /* write chunk (bb-be), data - O(1) */
  void put(int bb, int be, std::array<char, CHUNK_SIZE> data) 
  {
      auto k = key(bb,be);
       
      if(_cdata.size() == _size) {
        _lru_policy();
      }

      _update_key_last_access(k);

      _cdata.emplace(k, data);
  };
};
