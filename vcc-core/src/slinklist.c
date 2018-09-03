/*
 Copyright 2015 by Joseph Forgione
 This file is part of VCC (Virtual Color Computer).
 
 VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

/************************************************************************/
/**
	@defgroup slinklist Basic singly linked list
	@ingroup DataStructures
*/
/************************************************************************/

#include "slinklist.h"

#include "xDebug.h"

#include <stdio.h>

/************************************************************************/
/**
	@ingroup	slinklist
	
	Clears all elements from the singly linked list.
	
	@param[in,out]	llist	List to be cleared.
*/
void slinklistClear(slinklist_t * llist)
{
	assert(llist != NULL);
	
	llist->head	= NULL;
	llist->tail	= NULL;
	llist->count= 0;
}

/************************************************************************/
/**
	@ingroup	slinklist
	
	Adds an element to the front of the list.
	
	@param[in,out]	llist	List to add to.
	@param[in]		slink	Element to add to the list.
*/
void slinklistAddToHead(slinklist_t * llist, slink_t * slink)
{
	assert( llist != NULL );
	assert( slink != NULL );
	
	slink->next = llist->head;
	llist->head = slink;
	++llist->count;
	
	/* If there is no tail, this new link becomes it. */
	if ( llist->tail == NULL )
	{
		llist->tail = slink;
	}
}

/************************************************************************/
/**
	@ingroup	slinklist

	Adds an element to the back of the list.
	
	@param[in,out]	llist	List to add to.
	@param[in]		slink	Element to add to the list.
*/
void slinklistAddToTail(slinklist_t * llist, slink_t * slink)
{
	assert( llist != NULL );
	assert( slink != NULL );
	
	if ( llist->tail != NULL )
	{
		llist->tail->next = slink;
		llist->tail = slink;
		slink->next = NULL;
	}
	else
	{
		/* If there is no tail, there also shouldn't be a head. */
		assert( llist->head == NULL );
		
		llist->tail = slink;
		llist->head = slink;
		slink->next = NULL;
	}
	
	++llist->count;
}

/************************************************************************/
/**
	@ingroup	slinklist
*/
slink_t * slinklistPopFront(slinklist_t * llist)
{
	slink_t *	slink;
	
	assert( llist != NULL );
	
	slink = slinklistGetHead(llist);
	if ( slink != NULL )
	{
		slinklistRemove(llist, slink);
	}
	
	return slink;
}

/************************************************************************/
/**
	@ingroup	slinklist
*/
int slinklistRemove(slinklist_t * llist, slink_t * slink)
{
	int	        errResult = -1;
	slink_t *	pIter;
	
	assert( llist != NULL );
	assert( slink != NULL );
	
	/* If it's the head, there is no need to search through the list. */
	if ( slink == llist->head )
	{
		llist->head = slink->next;
		
		if ( slink == llist->tail )
		{
			/* If the link is both the head and the tail, the next pointer
			   must be NULL since it is the only element in the list. */
			assert(slink->next == NULL);
			llist->tail = NULL;
		}
		
		--llist->count;
		errResult = 0;
	}
	else
	{
		/* Starting from the front of the list, search until the link is 
		   found as the next pointer for the iterator. */
		pIter = llist->head;
		while ( pIter != NULL && pIter->next != slink )
		{
			pIter = pIter->next;
		}
		
		if ( pIter != NULL )
		{
			pIter->next = slink->next;
			if ( slink == llist->tail )
			{
				llist->tail = pIter;
			}
			
			--llist->count;
			errResult = 0;
		}
	}
	
	return errResult;
}

/************************************************************************/
/**
	@ingroup	slinklist
*/
slink_t * slinklistGetHead(slinklist_t * llist)
{
	assert( llist != NULL );
	return llist->head;
}

/************************************************************************/
/**
	@ingroup	slinklist
*/
slink_t * slinklistGetTail(slinklist_t * llist)
{
	assert( llist != NULL );
	return llist->tail;
}

/************************************************************************/
/**
	@ingroup	slinklist
*/
int slinklistGetCount(slinklist_t * llist)
{
	assert( llist != NULL );
	return llist->count;
}

/************************************************************************/
/**
	@ingroup	slinklist
*/
slink_t * slinklistGetNext(slink_t * slink)
{
	assert(slink != NULL);
	return slink->next;
}

/************************************************************************/
