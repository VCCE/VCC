#ifndef _slinklist_h_
#define _slinklist_h_

/************************************************************************/

typedef struct _slink_t_ slink_t;
struct _slink_t_
{
	slink_t *	next;
} ;

/************************************************************************/

typedef struct _slinklist_t_
{
	slink_t *	head;
	slink_t *	tail;
	
	int 		count;
} slinklist_t;

/************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	void		slinklistClear(slinklist_t * llist);

	void		slinklistAddToHead(slinklist_t * llist, slink_t * slink);
	void		slinklistAddToTail(slinklist_t * llist, slink_t * slink);
	slink_t *	slinklistPopFront(slinklist_t * llist);
	int	        slinklistRemove(slinklist_t * llist, slink_t * slink);

	slink_t *	slinklistGetHead(slinklist_t * llist);
	slink_t *	slinklistGetTail(slinklist_t * llist);
	int 		slinklistGetCount(slinklist_t * llist);
	slink_t *	slinklistGetNext(slink_t * slink);

#ifdef __cplusplus
}
#endif

/************************************************************************/

#endif
