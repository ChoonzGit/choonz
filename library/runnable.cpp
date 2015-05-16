// $Id: //adrian/choonz/choonz_library/main/runnable.cpp#2 $
// $Change: 64 $

#include "runnable.h"

void Runnable::operator () ()
{
	run ();
}

bool Runnable::isIdle () const
{
	return m_isIdle;
}

void Runnable::halt ()
{
	m_continue = false;
}

