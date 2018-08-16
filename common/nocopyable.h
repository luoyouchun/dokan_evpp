#ifndef __NOCOPYABLE_H_
#define __NOCOPYABLE_H_

namespace lpp
{
    //  Private copy constructor and copy assignment ensure classes derived from
    //  class noncopyable cannot be copied.

    //  Contributed luoyouchun
     class noncopyable
     {
     protected:
         noncopyable() {}
         ~noncopyable() {}
     private:  // emphasize the following members are private
         noncopyable(const noncopyable&);
         const noncopyable& operator=(const noncopyable&);
     };
}


typedef lpp::noncopyable noncopyable;

#endif
