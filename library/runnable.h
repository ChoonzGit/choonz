// $Id: //adrian/choonz/choonz_library/main/runnable.h#2 $ 
// $Change: 64 $
// An ABC for anything which runs under a boost::thread

#ifndef RUNNABLE_H_DEFINED
#define RUNNABLE_H_DEFINED

class Runnable
{
public:
	Runnable () : m_isIdle (true), m_continue (true) {}
	virtual ~Runnable () {}
	void operator () ();
	virtual void run () = 0;
	bool isIdle () const;
	void halt ();

protected:
	bool	m_isIdle;
	bool	m_continue; // Set to false to stop
};

#endif
