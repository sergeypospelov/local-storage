//
// Created by pospelov on 09.12.2021.
//

#ifndef LOCAL_STORAGE_INCLUDE_ISTARTABLE_H_
#define LOCAL_STORAGE_INCLUDE_ISTARTABLE_H_

class IRunnable {
public:
  virtual void start() = 0;
  virtual void stop() = 0;
  virtual bool isRunning() const = 0;

  virtual ~IRunnable() = default;
};


#endif // LOCAL_STORAGE_INCLUDE_ISTARTABLE_H_
