#include <iostream>
#include<vector>
#include<map>
#include <boost/unordered_map.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio.hpp>
#include <boost/unordered_map.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


using namespace std;

class ThreadPool
{
  public :
    ThreadPool( int noOfThreads = 1) ;
    ~ThreadPool() ;

    template< class func >
      void post( func f ) ;

    boost::asio::io_service &getIoService() ;

  private :
    boost::asio::io_service _ioService;
    boost::asio::io_service::work _work ;
    boost::thread_group _threads;
};

  inline ThreadPool::ThreadPool( int noOfThreads )
: _work( _ioService )
{
  for(int i = 0; i < noOfThreads ; ++i) // 4
    _threads.create_thread(boost::bind(&boost::asio::io_service::run, &_ioService));
}

inline ThreadPool::~ThreadPool()
{
  _ioService.stop() ;
  _threads.join_all() ;
}

inline boost::asio::io_service &ThreadPool::getIoService()
{
  return _ioService ;
}

  template< class func >
void ThreadPool::post( func f )
{
  _ioService.post( f ) ;
}

///Timer class to schedule callbacks triggered with timer, 
/// and evaluted by group of threads.
template<typename Func,typename CONTAINER>
class Timer{

  public:

    Timer(CONTAINER* container,Func callback,
        boost::asio::io_service& ioService,int expiry,int threadCount)
      : _timer(ioService)
    {
      _callBack = callback;
      _container = container;

      _pool = new ThreadPool(threadCount);

      _timer.expires_from_now(
          boost::posix_time::seconds((expiry)) ) ;

      _timer.async_wait(
          boost::bind(
            &Timer<Func,CONTAINER>::jobsExecution,this,
            boost::asio::placeholders::error ) ) ;
    }

  private:

    boost::asio::deadline_timer _timer;

    ThreadPool* _pool;

    Func _callBack;
    CONTAINER* _container;

    int _jobSize;

    void jobsExecution(const boost::system::error_code &error)
    {
      if( error != boost::asio::error::operation_aborted )
      {
        for(typename CONTAINER::iterator itr= _container->begin();
            itr!=_container->end();++itr)
        {
          _pool->post(boost::bind(_callBack,*itr));
          sleep(1);
        }
      }
      else
        cout<<error.message()<<endl;
    }
};

class Worker1{

  public:
    typedef void (*FPTR) (int);
    int x;

    Worker1()
    {
      _workVector.push_back(1);
    }

    static void job(int x)
    {
      cout<<"Wrk1 doing work..."<<x<<endl<<endl;
    } 

    vector<int>* getVector(){return &_workVector;}

    vector<int> _workVector;
};

template<class T>
class Worker2{

  public:
    typedef void (*FPTR) (T);
    int x;

    Worker2()
    {
    }

    void setData(T data)
    {
      _work.insert(data);
    }

    static void job(T x)
    {
      cout<<"Wrkr2 doing work Job1..."<<x<<endl<<endl;
    }

    static void job2(T x)
    {
      cout<<"Wrk2 doing work Job2..."<<x<<endl<<endl;
    }

    set<T> _work;

    set<T>* getSet(){return &_work;}
};


typedef void (*FuncPtr) (pair<string,boost::unordered_map<string,string> >);
boost::unordered_map<string,boost::unordered_map<string,string> > testMap;

set<int> testSet;

void show(pair<string,boost::unordered_map<string,string> > item)
{
  cout<<item.first<<"= ";
  for(boost::unordered_map<string,string> ::iterator itr = (item.second).begin();
      itr != (item.second).end();++itr)
  {
    cout<<(itr->first)<<",";
    cout<<(itr->second)<<endl;
  }
}


int main()
{

  boost::asio::io_service ioService;

  Worker1 wrk1;
  Timer<Worker1::FPTR,vector<int> > t1(wrk1.getVector(),
  Worker1::job,ioService,5,1);

  Worker2<int> wrk2;
  wrk2.setData(1);
  wrk2.setData(2);
  wrk2.setData(3);
  Timer<Worker2<int>::FPTR, set<int> > t2(wrk2.getSet(),
      Worker2<int>::job,ioService,8,2);

  Worker2<string> wrk3;
  wrk3.setData("Hello");
  wrk3.setData("World");
  Timer<Worker2<string>::FPTR, set<string> > t3(wrk3.getSet()
      ,Worker2<string>::job2,ioService,12,3);

  testMap["0"]["0"]="1";
  testMap["1"]["1"]="2";
  testMap["2"]["2"]="3";

  FuncPtr func = show;
  Timer<FuncPtr, boost::unordered_map<string,boost::unordered_map<string,string> > > t4(&testMap
  ,func,ioService,4,3);

  ioService.run();
  while(1)
  {
  }
  return 0;
}
