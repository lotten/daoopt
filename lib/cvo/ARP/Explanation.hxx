#ifndef ARE_Explanation_HXX_INCLUDED
#define ARE_Explanation_HXX_INCLUDED

#include <stdlib.h>

#ifdef WINDOWS
#include <windows.h>
#endif // WINDOWS

namespace ARE { class ARP ; }
namespace BucketElimination { class Bucket ; }

namespace ARE
{

typedef enum
{
	Explantion_NONE = 0,
	Explantion_Error
} ExplanationType ;

class Explanation
{
public :
	ExplanationType _Type ;
	BucketElimination::Bucket *_Bucket ;
	__int64 _FTBidx ;
	Explanation *_Next ;

public :

	Explanation(ExplanationType E = Explantion_NONE)
		:
		_Type(E), 
		_Bucket(NULL), 
		_FTBidx(0),
		_Next(NULL)
	{
	}
} ;

} // namespace ARE

#endif // ARE_Explanation_HXX_INCLUDED
