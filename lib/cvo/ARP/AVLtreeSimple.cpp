/*
	Module AVLTreeSimple.CPP

	This module contains the implementation of a simple AVL tree.

	Kalev Kask, March 2002.
*/

/*
	Regular MS C++ include files.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "AVLtreeSimple.hxx"
#include "Sort.hxx"

// random number generator
#include "MersenneTwister.h"

// Windows uses snprintf_s.  C99 standard uses snprintf.
#ifndef _WIN32
#define sprintf_s snprintf
#endif

static MTRand mtrandgen(4245) ;

#ifdef DEBUG_AVL_mutex
#include "util/OptexEXT.h"
#endif // DEBUG_AVL_mutex

/*
	Local definitions.
*/

//#define DEBUG_AVL

#ifdef _DEBUG
//#define DEBUG_AVL_new
//#define DEBUG_AVL_mutex
#endif // _DEBUG

#ifdef DEBUG_AVL_new
CMauiAVLTreeSimple *_msTestAVLTree = NULL ;
#endif // DEBUG_AVL_new


#ifdef DEBUG_AVL_mutex
COptexEXT_debug *_msAVLdebugMutex = NULL ;
#endif // _DEBUG

/*
	********** ********** ********** ********** ********** ********** ********** ********** **********
	CMauiAVLTreeSimple member functions.
	********** ********** ********** ********** ********** ********** ********** ********** **********
*/

void MauiAVLNodeSimple::operator<<(const _MauiAVLNodeSimple & Node)
{
	m_id = Node.m_id ;
	m_prev_entity = Node.m_prev_entity ;
	m_next_entity = Node.m_next_entity ;
	m_LC = Node.m_LC ;
	m_RC = Node.m_RC ;
	m_height = Node.m_height ;
	m_balance = Node.m_balance ;
	m_in_use = Node.m_in_use ;
}


void CMauiAVLTreeSimple::EmptyQuick(void)
{
	if (0 == m_size) 
		return ;

	unsigned long i, j ;

	// find last node in the used list
	j = m_root ;
	for (i = m_root ; 0 != i ; i = m_db_tree[i].m_RC) 
		j = i ;

	// hook up used list to unused list
	m_db_tree[j].m_next_entity = m_first_free_block ;
	m_first_free_block = m_first_used_block ;
	m_first_used_block = 0 ;
	m_size = 0 ;
	m_free_size = m_allocated_space ;
	m_root = 0 ;
}


/*
	This will empty the AVL-tree.
*/

void CMauiAVLTreeSimple::Empty(void)
{
	unsigned long i ;

/*
	Initialize parameters.
*/
	m_size = 0 ;
	m_free_size = m_allocated_space ;
	m_first_free_block = 1 ;
	m_first_used_block = 0 ;
	m_root = 0 ;

/*
	set up empty space
*/
	if (m_db_tree) {
		m_db_tree[m_first_free_block].m_in_use = 0 ;
		for (i = m_first_free_block + 1 ; i <= m_allocated_space ; i++) {
			m_db_tree[i-1].m_next_entity = i ;
			m_db_tree[i].m_in_use = 0 ;
			}
		m_db_tree[m_allocated_space].m_next_entity = 0 ;
		}
}


void CMauiAVLTreeSimple::SetReallocationSize(unsigned long reallocation_size)
{
	if (reallocation_size <= AVL_SIZE_LIMIT) 
		m_re_allocation_size = reallocation_size ;
}


/*
	This is an internal function to reserve space for the AVL tree.
*/

int CMauiAVLTreeSimple::AllocateMemory(unsigned long new_size)
{
	unsigned long i ;

	// check if there is a tree already. if yes, destroy it.
	if (m_db_tree) {
		delete[] m_db_tree ;
		m_allocated_space = 0 ;
		}

	if (new_size > 0) 
		m_db_tree = new MauiAVLNodeSimple[new_size] ;
	if (NULL == m_db_tree) {
		m_allocated_space = 0 ;
		m_size = 0 ;
		m_free_size = 0 ;
		m_first_free_block = 0 ;
		m_first_used_block = 0 ;
		m_root = 0 ;

		// not enough memory, return failure
		return 0 ;
		}

	m_allocated_space = new_size - 1 ; // subtract 1, since used indeces in db_tree start from 1.
	m_size = 0 ;
	m_free_size = m_allocated_space ;
	m_first_free_block = 1 ;
	m_first_used_block = 0 ;
	m_root = 0 ;

	// this is used as a dummy node when checking is not done whether a node has children or not
	m_db_tree[0].m_height = -1 ;

	// these are 'just-in-case' assignments
	m_db_tree[0].m_id = 0 ;
	m_db_tree[0].m_prev_entity = m_db_tree[0].m_next_entity = 0 ;
	m_db_tree[0].m_LC = m_db_tree[0].m_RC = 0 ;
	m_db_tree[0].m_balance = 0 ;
	m_db_tree[0].m_in_use = 0 ;

	// set up empty space
	m_db_tree[1].m_in_use = 0 ;
	for (i = 2 ; i <= m_allocated_space ; i++) {
		m_db_tree[i-1].m_next_entity = i ;
		m_db_tree[i].m_in_use = 0 ;
		}
	m_db_tree[m_allocated_space].m_next_entity = 0 ;

	return 1 ;
}


/*
	When the tree is full and we Insert(), system tries to reallocate the tree.
*/

int CMauiAVLTreeSimple::ReAllocateMemory(void)
{
	unsigned long new_size, i ;
	MauiAVLNodeSimple *new_db_tree = NULL ;

	if (m_re_allocation_size < 1) return 0 ;

	if (NULL == m_db_tree) {
		// if the current tree is NULL, use AllocateMemory()
		return AllocateMemory(m_re_allocation_size + 1) ;
		}

	// compute the size of the new tree
	i = m_allocated_space + 1 ;
	new_size = i + m_re_allocation_size ;
	// allocate memory for the new tree
	new_db_tree = new MauiAVLNodeSimple[new_size] ;
	if (NULL == new_db_tree) return 0 ;

	// copy the entire old tree into the new tree
	memcpy(new_db_tree,m_db_tree,(size_t) i*sizeof(MauiAVLNodeSimple)) ;

	// set up empty space as a linked list
	if (m_first_free_block > 0) new_db_tree[m_first_free_block].m_next_entity = i ;
	else m_first_free_block = i ;
	new_db_tree[i].m_in_use = 0 ;
	for (++i ; i < new_size ; i++) {
		new_db_tree[i-1].m_next_entity = i ;
		new_db_tree[i].m_in_use = 0 ;
		}
	new_db_tree[new_size-1].m_next_entity = 0 ;

	// delete old tree
	delete[] m_db_tree ;

	// assign the pointer to the new tree to db_tree
	m_db_tree = new_db_tree ;
	new_db_tree = NULL ;
	m_allocated_space = new_size - 1 ;
	m_free_size += m_re_allocation_size ;

	return 1 ;
}


/*
	This default constructor creates an empty tree of size AVL_DEFAULT_SIZE.
*/

CMauiAVLTreeSimple::CMauiAVLTreeSimple(void)
{
	m_db_tree = NULL ;
	if (! AllocateMemory(AVL_DEFAULT_SIZE)) return ;

	m_re_allocation_size = AVL_DEFAULT_SIZE ;
}


/*
	This function is the strandard constructor.

	It creates an AVL tree from a given set of keys.
	Keys are in the array 'key'.
	'size' is the number of keys and 'data' contains information associated with every key.

	Assumptions :
		- Arrays 'key' and 'data' start from index 0.
		- A key with the highest priority is the one that has the largest key value.

	It is a good idea to reserve enough space for the heap so that it does not have to be
	reallocated very often.

	NB! this function will sort the array 'key' (and 'data').
*/

CMauiAVLTreeSimple::CMauiAVLTreeSimple(unsigned long tree_size /* number of keys (>= 0) */, 
	long *key /* an array of keys */, 
	unsigned long space /* how much space to allocate for the AVL tree (> 0) */, 
	unsigned long re_allocation_size)
{
	unsigned long i ;
	long j ;

/*
	Qualification check.
*/
	if (space < 1) space = AVL_DEFAULT_SIZE ;
	if (tree_size > 0 && NULL == key) tree_size = 0 ;
	if (space < tree_size) space = tree_size ;
	// AVL tree will start from index 1, not 0; therefore we need one more array-element
	++space ;
	// too many keys. cut it short.
	if (tree_size >= AVL_SIZE_LIMIT) {
		tree_size = AVL_SIZE_LIMIT - 1 ;
		space = tree_size + 1 ;
		}
	// limit the max size of the AVL tree
	if (space > AVL_SIZE_LIMIT) {
		space = AVL_SIZE_LIMIT ;
		}

	m_re_allocation_size = re_allocation_size ;

/*
	Allocate memory for the AVL tree. If a tree exists currently, it will be destroyed.
*/
	m_db_tree = NULL ;
	if (! AllocateMemory(space)) {
		return ;
		}

/* ********** ********** *
 * sort entities first
 * ********** ********** */

	long left[32], right[32] ;
	QuickSortLong2(key,tree_size, left, right) ;

#ifdef DEBUG_AVL
	if (! Sort_Check_long(key,tree_size)) {
		}
#endif

/*
	Copy keys and data into the AVL tree.
*/
	m_size = 0 ;
	for (i = 0 ; i < tree_size ; i++) {
		m_db_tree[++m_size].m_id = key[i] ;
		m_db_tree[m_size].m_in_use = 1 ;
		}

/* ********** ********** ********** ********** ********** *
 * set up linked lists of used and unused blocks
 * ********** ********** ********** ********** ********** */

	m_free_size = m_allocated_space - m_size ;
	m_first_free_block = m_size + 1 ;
	if (m_size > 0) m_first_used_block = 1 ;

/*
	Set up empty space.
*/
	if (m_free_size > 0) {
		m_db_tree[m_first_free_block].m_in_use = 0 ;

		for (i = m_first_free_block + 1 ; i <= m_allocated_space ; i++) {
			m_db_tree[i-1].m_next_entity = i ;
			m_db_tree[i].m_in_use = 0 ;
			}

		m_db_tree[m_allocated_space].m_next_entity = 0 ;
		}

/*
	If the AVL tree is empty we are done.
*/
	if (m_size < 1) return ;

/*
	set up used space as a linked list
*/
	m_db_tree[1].m_prev_entity = 0 ;
	m_db_tree[m_size].m_next_entity = 0 ;
	for (i = 1 ; i <= m_size ; i++) {
		m_db_tree[i].m_prev_entity = i - 1 ;
		m_db_tree[i-1].m_next_entity = i ;
		}

/* ********** ********** *
 * build an AVL-tree
 * ********** ********** */

	m_root = m_size >> 1 ;
	++m_root ; // this will be the index of the root node
	Left[0] = 1 ;
	Right[0] = m_size ;
	Middle[0] = m_root ;
	j = 0 ;

#ifdef DEBUG_AVL
//	print("root address is %d\n",root) ;
#endif

	while (j >= 0) {
	// idea: Middle[], Left[] and Right[] act as a stack
	// we traverse the tree recursively, left subtree first
	// every time the parent information is stored in the stack (Middle[], Left[], Right[])
	// j is the stack pointer
		if (Left[j]) { // handle left side
			i = (Middle[j] - Left[j]) >> 1 ;
			i += Left[j] ;
			m_db_tree[Middle[j]].m_LC = i ; // a pointer from the parent to the left child
			++j ;
			Middle[j] = i ;
			Left[j] = Left[j - 1] ;
			Right[j] = Middle[j - 1] - 1 ;
			if (Left[j] >= Middle[j]) {
				m_db_tree[i].m_LC = 0 ;
				Left[j] = 0 ;
				}
			if (Right[j] <= Middle[j]) {
				m_db_tree[i].m_RC = 0 ;
				Right[j] = 0 ;
				}
			}
		else if (Right[j]) { // handle right side
			i = (Right[j] - Middle[j]) >> 1 ;
			i += 1 + Middle[j] ;
			m_db_tree[Middle[j]].m_RC = i ;
			++j ;
			Middle[j] = i ;
			Left[j] = Middle[j - 1] + 1 ;
			Right[j] = Right[j - 1] ;
			if (Left[j] >= Middle[j]) {
				m_db_tree[i].m_LC = 0 ;
				Left[j] = 0 ;
				}
			if (Right[j] <= Middle[j]) {
				m_db_tree[i].m_RC = 0 ;
				Right[j] = 0 ;
				}
			}
		else { // both sides done -> backtrack
			if (! m_db_tree[Middle[j]].m_LC) { // no left subtree
				if (! m_db_tree[Middle[j]].m_RC) m_db_tree[Middle[j]].m_height = 0 ;
				else m_db_tree[Middle[j]].m_height = m_db_tree[m_db_tree[Middle[j]].m_RC].m_height + 1 ;
				m_db_tree[Middle[j]].m_balance = (signed char) m_db_tree[Middle[j]].m_height ;
				}
			else if (! m_db_tree[Middle[j]].m_RC) { // left subtree, but no right subtree
				m_db_tree[Middle[j]].m_height = m_db_tree[m_db_tree[Middle[j]].m_LC].m_height + 1 ;
				m_db_tree[Middle[j]].m_balance = (signed char) (-m_db_tree[Middle[j]].m_height) ;
				}
			else { // has both subtrees
				m_db_tree[Middle[j]].m_balance = (signed char) (m_db_tree[m_db_tree[Middle[j]].m_RC].m_height - m_db_tree[m_db_tree[Middle[j]].m_LC].m_height) ;
				m_db_tree[Middle[j]].m_height = (m_db_tree[m_db_tree[Middle[j]].m_RC].m_height > m_db_tree[m_db_tree[Middle[j]].m_LC].m_height) ? 
					m_db_tree[m_db_tree[Middle[j]].m_RC].m_height : m_db_tree[m_db_tree[Middle[j]].m_LC].m_height ;
				++m_db_tree[Middle[j]].m_height ;
				}

			--j ;
			if (j < 0) break ;
			if (Left[j]) Left[j] = 0 ;
			else Right[j] = 0 ;
			}
		}
}


/*
	Destructor has to release the memory allocated for the AVL tree.
*/

CMauiAVLTreeSimple::~CMauiAVLTreeSimple(void)
{
	if (m_db_tree) delete[] m_db_tree ;
}


void CMauiAVLTreeSimple::PrintTree(void)
{
	// TODO.
}


/*
	Check if the AVL tree is internally consistent.

	Return FALSE if not, otherwise TRUE.
*/

#define SIZE_CHECK_TREE 1024
static char sCheckTree[SIZE_CHECK_TREE] ;
long CMauiAVLTreeSimple::CheckTree(char **pErrorStr)
{
	unsigned long i, j, m ;
	unsigned long k ;

	if (pErrorStr) 
		*pErrorStr = 0 ;

/*
	Qualification check.
*/
	if (NULL == m_db_tree) return 1 ;

	if (m_db_tree[0].m_height != 0) {
		if (pErrorStr) {
			sprintf_s(sCheckTree, SIZE_CHECK_TREE, "tree[0] height is not 0 (is %d)\n", m_db_tree[0].m_height) ;
			*pErrorStr = sCheckTree ;
			}
		goto failure_12_09_1994 ;
		}

	if (m_size < 1) {
		if (m_free_size != m_allocated_space) {
			if (pErrorStr) {
				sprintf_s(sCheckTree, SIZE_CHECK_TREE, "db size is < 1, free size is not VLM_DB_SIZE-1\n") ;
				*pErrorStr = sCheckTree ;
				}
			return 0 ;
			}
		if (m_root != 0) {
			if (pErrorStr) {
				sprintf_s(sCheckTree, SIZE_CHECK_TREE, "size is < 1, root pointer is not 0\n") ;
				*pErrorStr = sCheckTree ;
				}
			return 0 ;
			}
		}

	if (m_size + m_free_size != m_allocated_space) {
		if (pErrorStr) {
			sprintf_s(sCheckTree, SIZE_CHECK_TREE, "the sum of used and free blocks does not match VLM_DB_SIZE-1\n") ;
			*pErrorStr = sCheckTree ;
			}
		goto failure_12_09_1994 ;
		}

	// check if the next/prev entity lists are right 
	k = 0 ;
	for (i = m_first_used_block, j = 0 ; i ; i = m_db_tree[i].m_next_entity) {
		if (++j > m_size) {
			if (pErrorStr) {
				sprintf_s(sCheckTree, SIZE_CHECK_TREE, "next_entity list too long\n") ;
				*pErrorStr = sCheckTree ;
				}
			goto failure_12_09_1994 ;
			}
		if (i < 1 || i > m_allocated_space) {
			if (pErrorStr) {
				sprintf_s(sCheckTree, SIZE_CHECK_TREE, "next entity list pointers out of bounds\n") ;
				*pErrorStr = sCheckTree ;
				}
			goto failure_12_09_1994 ;
			}
		k = i ;
		}
	if (j != m_size) {
		if (pErrorStr) {
			sprintf_s(sCheckTree, SIZE_CHECK_TREE, "next entity list does not contain size number of blocks\n") ;
			*pErrorStr = sCheckTree ;
			}
		goto failure_12_09_1994 ;
		}
	for (i = k, j = 0 ; i ; i = m_db_tree[i].m_prev_entity) {
		if (++j > m_size) {
			if (pErrorStr) {
				sprintf_s(sCheckTree, SIZE_CHECK_TREE, "prev_entity list too long\n") ;
				*pErrorStr = sCheckTree ;
				}
			goto failure_12_09_1994 ;
			}
		if (i < 1 || i > m_allocated_space) {
			if (pErrorStr) {
				sprintf_s(sCheckTree, SIZE_CHECK_TREE, "prev entity list pointers out of bounds\n") ;
				*pErrorStr = sCheckTree ;
				}
			goto failure_12_09_1994 ;
			}
		}
	if (j != m_size) {
		if (pErrorStr) {
			sprintf_s(sCheckTree, SIZE_CHECK_TREE, "prev entity list does not contain size number of blocks\n") ;
			*pErrorStr = sCheckTree ;
			}
		goto failure_12_09_1994 ;
		}

	// check if the free blocks list is right
	for (i = m_first_free_block, j = 0 ; i ; i = m_db_tree[i].m_next_entity) {
		if (++j > m_free_size) {
			if (pErrorStr) {
				sprintf_s(sCheckTree, SIZE_CHECK_TREE, "free next_entity list too long\n") ;
				*pErrorStr = sCheckTree ;
				}
			goto failure_12_09_1994 ;
			}
		if (i < 1 || i > m_allocated_space) {
			if (pErrorStr) {
				sprintf_s(sCheckTree, SIZE_CHECK_TREE, "free next entity list pointers out of bounds\n") ;
				*pErrorStr = sCheckTree ;
				}
			goto failure_12_09_1994 ;
			}
		}
	if (j != m_free_size) {
		if (pErrorStr) {
			sprintf_s(sCheckTree, SIZE_CHECK_TREE, "free next entity list does not contain free_size number of blocks\n") ;
			*pErrorStr = sCheckTree ;
			}
		goto failure_12_09_1994 ;
		}

	// check if the number of in_use blocks is right
	for (i = 1, j = k = 0 ; i <= m_allocated_space ; i++) {
		if (m_db_tree[i].m_in_use) {
			j++ ;
			if (m_db_tree[i].m_RC > m_allocated_space ||m_db_tree[i].m_RC > m_allocated_space) {
				if (pErrorStr) {
					sprintf_s(sCheckTree, SIZE_CHECK_TREE, "block has LC or RC pointer(s) out of bounds\n") ;
					*pErrorStr = sCheckTree ;
					}
				goto failure_12_09_1994 ;
				}
			}
		else k++ ;
		}
	if (j != m_size) {
		if (pErrorStr) {
			sprintf_s(sCheckTree, SIZE_CHECK_TREE, "number of in_use blocks does not match what size says\n") ;
			*pErrorStr = sCheckTree ;
			}
		goto failure_12_09_1994 ;
		}
	if (k != m_free_size) {
		if (pErrorStr) {
			sprintf_s(sCheckTree, SIZE_CHECK_TREE, "number of not in_use blocks not not match what free_size says\n") ;
			*pErrorStr = sCheckTree ;
			}
		goto failure_12_09_1994 ;
		}

/*
	Check if the tree satisfies binary tree property:
		Left will store the stack 
		Right will store the height
		k will be index in Left[], Right[]
		j will be number of nodes
		i will be "current" node
*/
	if (m_root < 1) goto skip_avl_binary_tree_property_check ;
	Left[0] = m_root ;
	Right[0] = k = j = 0 ;
	while (k >= 0) {
		++j ;
		if (j > m_size) {
			if (pErrorStr) {
				sprintf_s(sCheckTree, SIZE_CHECK_TREE, "tree structure wrong -> there seems to be more blocks than actually possible") ;
				*pErrorStr = sCheckTree ;
				}
			goto failure_12_09_1994 ;
			}

		// if no children, backtrack.
		if (! m_db_tree[Left[k]].m_LC && ! m_db_tree[Left[k]].m_RC) {
backtrack_12_07_1994:
			m = Right[k] ;
			for (--k ; k >= 0 && Left[k] < 0 ; k--) {
				++m ;
				// right now: [k] points to a node; m is right child height, Right[k] is left child height
				if ((m - Right[k]) != (unsigned long) m_db_tree[labs(Left[k])].m_balance) {
					if (pErrorStr) {
						j = labs(Left[k]) ;
						sprintf_s(sCheckTree, SIZE_CHECK_TREE,"mismatch of balances real r=%ld l=%ld node=%ld h=%d known=%d LC h=%d RC h=%d",
							m, Right[k], j, 
							(int) m_db_tree[j].m_height, (int) m_db_tree[j].m_balance, 
							(int) m_db_tree[m_db_tree[j].m_LC].m_height, (int) m_db_tree[m_db_tree[j].m_RC].m_height) ;
						*pErrorStr = sCheckTree ;
						}
					goto failure_12_09_1994 ;
					}
				if (Right[k] > (long)m) m = Right[k] ;
				if (m != (unsigned long) m_db_tree[labs(Left[k])].m_height) {
					if (pErrorStr) {
						sprintf_s(sCheckTree, SIZE_CHECK_TREE,"real height is not equal to what is known in db; 2--> k=%ld Right[k]=%ld Left[k]=%ld m_db_tree[Left[k]]=%d\n",k,Right[k],Left[k],m_db_tree[labs(Left[k])].m_height) ;
						*pErrorStr = sCheckTree ;
						}
					goto failure_12_09_1994 ;
					}
				}
			++m ;
			if (Right[k] < (long)m) Right[k] = m ;
			else m = Right[k] ;
			if (k < 0) break ; // done
			// we used to go left at node [k] (because left[k]>0); now we need to go right.
			Left[k] = -Left[k] ;
			}

		i = labs(Left[k]) ;
		// check that the computation of balance is done correctly, ie. numbers in db-tree match
		if (m_db_tree[i].m_RC) {
			if (m_db_tree[i].m_RC > m_allocated_space) {
				if (pErrorStr) {
					sprintf_s(sCheckTree, SIZE_CHECK_TREE,"RC out of bounds") ;
					*pErrorStr = sCheckTree ;
					}
				goto failure_12_09_1994 ;
				}

			if (m_db_tree[i].m_LC) {
				if (m_db_tree[i].m_LC > m_allocated_space) {
					if (pErrorStr) {
						sprintf_s(sCheckTree, SIZE_CHECK_TREE,"LC out of bounds") ;
						*pErrorStr = sCheckTree ;
						}
					goto failure_12_09_1994 ;
					}

				if (((long) m_db_tree[i].m_balance) != ((long) m_db_tree[m_db_tree[i].m_RC].m_height) - ((long) m_db_tree[m_db_tree[i].m_LC].m_height)) {
					if (pErrorStr) {
						sprintf_s(sCheckTree, SIZE_CHECK_TREE,"1 balance computed incorrectly. node %ld",i) ;
						*pErrorStr = sCheckTree ;
						}
					goto failure_12_09_1994 ;
					}
				}
			else {
				if (((long) m_db_tree[i].m_balance) != ((long) m_db_tree[m_db_tree[i].m_RC].m_height) + 1) {
					if (pErrorStr) {
						sprintf_s(sCheckTree, SIZE_CHECK_TREE,"2 balance computed incorrectly. node %ld",i) ;
						*pErrorStr = sCheckTree ;
						}
					goto failure_12_09_1994 ;
					}
				}
			}
		else {
			if (m_db_tree[i].m_LC > m_allocated_space) {
				if (pErrorStr) {
					sprintf_s(sCheckTree, SIZE_CHECK_TREE,"LC out of bounds") ;
					*pErrorStr = sCheckTree ;
					}
				goto failure_12_09_1994 ;
				}

			if (((long) m_db_tree[i].m_balance) != -((long) m_db_tree[m_db_tree[i].m_LC].m_height) - 1) {
				if (pErrorStr) {
					sprintf_s(sCheckTree, SIZE_CHECK_TREE,"3 balance computed incorrectly. node %ld",i) ;
					*pErrorStr = sCheckTree ;
					}
				goto failure_12_09_1994 ;
				}
			}

		// traverse left/right subtree (expand to left/right child)
		if (m_db_tree[i].m_LC && Left[k] > 0) {
			Left[++k] = m_db_tree[i].m_LC ;
			}
		else { // right subtree (if exists) not scanned
			if (0 == m_db_tree[i].m_RC) { // no right subtree!
				if (-Right[k] != (long) m_db_tree[labs(Left[k])].m_balance) {
					if (pErrorStr) {
						sprintf_s(sCheckTree, SIZE_CHECK_TREE,"real balance is not what is known in db (no right subtree)") ;
						*pErrorStr = sCheckTree ;
						}
					goto failure_12_09_1994 ;
					}
				if (Right[k] != (long) m_db_tree[labs(Left[k])].m_height) {
					if (pErrorStr) {
						sprintf_s(sCheckTree, SIZE_CHECK_TREE,"real height is not equal to what is known in db (no right subtree); 2--> k=%ld Right[k]=%ld Left[k]=%ld m_db_tree[Left[k]]=%d\n",k,Right[k],Left[k],m_db_tree[labs(Left[k])].m_height) ;
						*pErrorStr = sCheckTree ;
						}
					goto failure_12_09_1994 ;
					}
				goto backtrack_12_07_1994 ;
				}
			if (Left[k] > 0) Left[k] = -Left[k] ;
			Left[++k] = m_db_tree[i].m_RC ;
			}

		// check the value of the 'balance' variable is within bounds
		if (m_db_tree[Left[k]].m_balance < ((signed char) -1) ||
			m_db_tree[Left[k]].m_balance > ((signed char) 1)) {
			if (pErrorStr) {
				sprintf_s(sCheckTree, SIZE_CHECK_TREE,"balance (%d) is out of bounds [k=%ld, Left[k]=%ld]\n",(int) m_db_tree[Left[k]].m_balance,k,Left[k]) ;
				*pErrorStr = sCheckTree ;
				}
			goto failure_12_09_1994 ;
			}

		// check that the keys are smaller in the left subtree and larger in the right subtree
		Right[k] = 0 ;
		for (m = 0 ; m < k; m++) {
			if (Left[m] > 0) {
				if (m_db_tree[Left[k]].m_id > m_db_tree[Left[m]].m_id) {
					if (pErrorStr) {
						sprintf_s(sCheckTree, SIZE_CHECK_TREE,"id in left subtree is greater than an ancestor; k=%ld m=%ld Left[k]=%ld Left[m]=%ld tree[Left[k]].m_id=%ld tree[..m..]=%ld\n",k,m,Left[k],Left[m],m_db_tree[Left[k]].m_id,m_db_tree[Left[m]].m_id) ;
						*pErrorStr = sCheckTree ;
						}
					goto failure_12_09_1994 ;
					}
				}
			else {
				if (m_db_tree[Left[k]].m_id < m_db_tree[-Left[m]].m_id) {
					if (pErrorStr) {
						sprintf_s(sCheckTree, SIZE_CHECK_TREE,"id in right subtree is less than an ancestor") ;
						*pErrorStr = sCheckTree ;
						}
					goto failure_12_09_1994 ;
					}
				}
			}
		}

skip_avl_binary_tree_property_check :

	return 1 ;

failure_12_09_1994:

#ifdef DEBUG_AVL
	PrintTree() ;
#endif

	return 0 ;
}


/* ********** ********** ********** ********** ********** ********** ********** ********** ********** *
 * local, non-exported functions
 * ********** ********** ********** ********** ********** ********** ********** ********** ********** */


void CMauiAVLTreeSimple::R_rotation(unsigned long k)
// make an R rotation (this function is used only by AVL_delete)
{
	unsigned long i, lc, rc ;

	i = m_db_tree[k].m_LC ;
	m_db_tree[k].m_LC = m_db_tree[i].m_RC ;
	m_db_tree[i].m_RC = k ;
	lc = m_db_tree[k].m_LC ;
	rc = m_db_tree[k].m_RC ;
	m_db_tree[k].m_balance = (signed char) (m_db_tree[rc].m_height - m_db_tree[lc].m_height) ;
	m_db_tree[k].m_height = (m_db_tree[lc].m_height > m_db_tree[rc].m_height) ?
		m_db_tree[lc].m_height : m_db_tree[rc].m_height ;
	++m_db_tree[k].m_height ;
	lc = m_db_tree[i].m_LC ;
	m_db_tree[i].m_balance = (signed char) (m_db_tree[k].m_height - m_db_tree[lc].m_height) ;
	m_db_tree[i].m_height = (m_db_tree[k].m_height > m_db_tree[lc].m_height) ?
		m_db_tree[k].m_height : m_db_tree[lc].m_height ;
	++m_db_tree[i].m_height ;
}


void CMauiAVLTreeSimple::L_rotation(unsigned long k)
// make an L rotation (this function is used only by AVL_delete)
{
	unsigned long i, lc, rc ;

	i = m_db_tree[k].m_RC ;
	m_db_tree[k].m_RC = m_db_tree[i].m_LC ;
	m_db_tree[i].m_LC = k ;
	lc = m_db_tree[k].m_LC ;
	rc = m_db_tree[k].m_RC ;
	m_db_tree[k].m_balance = (signed char) (m_db_tree[rc].m_height - m_db_tree[lc].m_height) ;
	m_db_tree[k].m_height = (m_db_tree[lc].m_height > m_db_tree[rc].m_height) ?
		m_db_tree[lc].m_height : m_db_tree[rc].m_height ;
	++m_db_tree[k].m_height ;
	rc = m_db_tree[i].m_RC ;
	m_db_tree[i].m_balance = (signed char) (m_db_tree[rc].m_height - m_db_tree[k].m_height) ;
	m_db_tree[i].m_height = (m_db_tree[k].m_height > m_db_tree[rc].m_height) ?
		m_db_tree[k].m_height : m_db_tree[rc].m_height ;
	++m_db_tree[i].m_height ;
}


void CMauiAVLTreeSimple::BalanceInsertTree(void)
// balance the tree
// Left[path_len] points to the current node. array 'Left' contains the path to the current node
// the node that was added is one of its children
// NB! this function is used when we insert a key
// 		when we delete a key, a different balancing function is used
//		(this function here is very specific for insertion (and perhaps a little more efficient than the general balancing function))
{
	unsigned long a, b, c, d, i ;

	if (path_len < 1) return ; // the tree is already balanced

	b = Left[path_len + 1] ;
	c = Left[path_len] ;
	a = Left[--path_len] ;
	d = 0 ;
	// a is a parent of c, which is a parent of b, which is a parent of d

up_the_tree_12_04_94:
	if (m_db_tree[a].m_RC == c) {
		++(m_db_tree[a].m_balance) ;
		if (m_db_tree[a].m_balance == 2) { // found a critical node
			if (m_db_tree[c].m_RC == b) { // single right rotation
				i = m_db_tree[c].m_LC ;
				m_db_tree[c].m_LC = a ;
				m_db_tree[a].m_RC = i ;
				m_db_tree[a].m_balance = m_db_tree[c].m_balance = 0 ;
				// NB! c's height does not change, a's does
				// c's new height is equal to a's old height
				// ie. no further rebalancing needs to be done
				--m_db_tree[a].m_height ;
				if (path_len < 1) m_root = c ;
				else {
					--path_len ;
					if (m_db_tree[Left[path_len]].m_LC == a) m_db_tree[Left[path_len]].m_LC = c ;
					else m_db_tree[Left[path_len]].m_RC = c ;
					}
				}
			else { // double rotation
				i = m_db_tree[b].m_RC ;
				m_db_tree[b].m_RC = c ;
				m_db_tree[c].m_LC = i ;
				i = m_db_tree[b].m_LC ;
				m_db_tree[b].m_LC = a ;
				m_db_tree[a].m_RC = i ; // pointers are rotated now
				m_db_tree[b].m_balance = 0 ;
				if (d == 0) m_db_tree[a].m_balance = m_db_tree[c].m_balance = 0 ;
				else {
					if (d == m_db_tree[c].m_LC) {
						m_db_tree[a].m_balance = -1 ;
						m_db_tree[c].m_balance = 0 ;
						}
					else {
						m_db_tree[a].m_balance = 0 ;
						m_db_tree[c].m_balance = 1 ;
						}
					} // balances are changed now
				++m_db_tree[b].m_height ;
				--m_db_tree[a].m_height ;
				--m_db_tree[c].m_height ; // height is updated now
				if (path_len < 1) m_root = b ;
				else {
					--path_len ;
					if (m_db_tree[Left[path_len]].m_LC == a) m_db_tree[Left[path_len]].m_LC = b ;
					else m_db_tree[Left[path_len]].m_RC = b ;
					} // the parent of a (in the old tree) is updated
				}
			return ;
			}
		else if (m_db_tree[a].m_balance == 0) return ; // found a critical node
		else { // balance is 1
			++m_db_tree[a].m_height ;
			if (path_len < 1) return ; // done, no critical node
			d = b ;
			b = c ;
			c = a ;
			a = Left[--path_len] ;
			goto up_the_tree_12_04_94 ;
			}
		}
	else {
		--(m_db_tree[a].m_balance) ;
		if (m_db_tree[a].m_balance == -2) { // found a critical node
			if (m_db_tree[c].m_LC == b) { // single left rotation
				i = m_db_tree[c].m_RC ;
				m_db_tree[c].m_RC = a ;
				m_db_tree[a].m_LC = i ;
				m_db_tree[a].m_balance = m_db_tree[c].m_balance = 0 ;
				--m_db_tree[a].m_height ;
				if (path_len < 1) m_root = c ;
				else {
					--path_len ;
					if (m_db_tree[Left[path_len]].m_LC == a) m_db_tree[Left[path_len]].m_LC = c ;
					else m_db_tree[Left[path_len]].m_RC = c ;
					}
				}
			else { // double rotation
				i = m_db_tree[b].m_LC ;
				m_db_tree[b].m_LC = c ;
				m_db_tree[c].m_RC = i ;
				i = m_db_tree[b].m_RC ;
				m_db_tree[b].m_RC = a ;
				m_db_tree[a].m_LC = i ; // pointers are rotated now
				m_db_tree[b].m_balance = 0 ;
				if (d == 0) m_db_tree[a].m_balance = m_db_tree[c].m_balance = 0 ;
				else {
// bug fix : next line used to be "if (d == m_db_tree[c].m_LC) {"
					if (d == m_db_tree[c].m_RC) {
						m_db_tree[a].m_balance = 1 ;
						m_db_tree[c].m_balance = 0 ;
						}
					else {
						m_db_tree[a].m_balance = 0 ;
						m_db_tree[c].m_balance = -1 ;
						}
					} // balances are changed now
				++m_db_tree[b].m_height ;
				--m_db_tree[a].m_height ;
				--m_db_tree[c].m_height ; // height is updated now
				if (path_len < 1) m_root = b ;
				else {
					--path_len ;
					if (m_db_tree[Left[path_len]].m_LC == a) m_db_tree[Left[path_len]].m_LC = b ;
					else m_db_tree[Left[path_len]].m_RC = b ;
					} // the parent of a (in the old tree) is updated
				}
			return ;
			}
		else if (m_db_tree[a].m_balance == 0) return ; // found a critical node
		else { // balance is -1
			++m_db_tree[a].m_height ;
			if (path_len < 1) return ; // done, no critical node
			d = b ;
			b = c ;
			c = a ;
			a = Left[--path_len] ;
			goto up_the_tree_12_04_94 ;
			}
		}
}


void CMauiAVLTreeSimple::BalanceDeleteTree(void)
// balance the tree after an deletion is done
{
	unsigned long i, j, k ;
	signed short h ;

	// Left[path_len] is equal to the current node (its balance and height are correct)
	// array 'Left' contains the path to the current node
	k = Left[path_len] ;

rebalance_parent_12_07_1994:
	h = m_db_tree[k].m_height ;
	if (m_db_tree[k].m_balance == 2) { // need a rotation (balance is -2 or +2)
		i = m_db_tree[k].m_RC ;
		if (m_db_tree[i].m_balance < 0) {
			j = m_db_tree[i].m_LC ;
			R_rotation(i) ;
			m_db_tree[k].m_RC = j ;
			}
		j = m_db_tree[k].m_RC ;
		L_rotation(k) ;
		if (path_len == 0) {
			m_root = j ;
			return ;
			}
		--path_len ;
		if (m_db_tree[Left[path_len]].m_LC == k) m_db_tree[Left[path_len]].m_LC = j ;
		else m_db_tree[Left[path_len]].m_RC = j ;

		if (h == m_db_tree[j].m_height) return ; // done
		}
	else if (m_db_tree[k].m_balance == -2) {
		i = m_db_tree[k].m_LC ;
		if (m_db_tree[i].m_balance > 0) {
			j = m_db_tree[i].m_RC ;
			L_rotation(i) ;
			m_db_tree[k].m_LC = j ;
			}
		j = m_db_tree[k].m_LC ;
		R_rotation(k) ;
		if (path_len == 0) {
			m_root = j ;
			return ;
			}
		--path_len ;
		if (m_db_tree[Left[path_len]].m_LC == k) m_db_tree[Left[path_len]].m_LC = j ;
		else m_db_tree[Left[path_len]].m_RC = j ;

		if (h == m_db_tree[j].m_height) return ; // done
		}
	else { // balance is 0 -> height of parent might have changed
		if (path_len == 0) {
			m_root = k ;
			return ;
			}
		--path_len ;
		}
	
	k = Left[path_len] ;
	h = m_db_tree[k].m_height ;
	m_db_tree[k].m_balance = (signed char) (m_db_tree[m_db_tree[k].m_RC].m_height - m_db_tree[m_db_tree[k].m_LC].m_height) ;
	m_db_tree[k].m_height = (m_db_tree[m_db_tree[k].m_RC].m_height > m_db_tree[m_db_tree[k].m_LC].m_height) ?
		m_db_tree[m_db_tree[k].m_RC].m_height : m_db_tree[m_db_tree[k].m_LC].m_height ;
	++m_db_tree[k].m_height ;
	if (h == m_db_tree[k].m_height && m_db_tree[k].m_balance <= 1 && m_db_tree[k].m_balance >= (signed char) -1) return ; // done

	goto rebalance_parent_12_07_1994 ;
}


/* ********** ********** ********** ********** ********** ********** ********** ********** ********** *
 * global, exported functions
 * ********** ********** ********** ********** ********** ********** ********** ********** ********** */

/*
	check if an entity is in the AVL tree
	in case it is not found, it will leave a trace of the path from the root
		to where it should be added in array 'Left' (so that this path 
 		can be used by insert and delete procedures)
	return either 0 (no such id) or the location (which is > 0) 
	if this id is not in the tree, Left[path_len] is either -1 (if this
		id should be the left child of the last node seen), or 1 (if this
		id should be the right child of the last node seen).
		If the id is in the tree, Left[path_len] is 0.
*/

long CMauiAVLTreeSimple::Find(long id)  
{
#ifdef DEBUG_AVL_mutex
	if (NULL == _msAVLdebugMutex) 
		_msAVLdebugMutex = new COptexEXT_debug("AVLtestMutex") ;
	COptexEXTAutoLock lock(*_msAVLdebugMutex) ;
#endif // _DEBUG

	long i, j ;

/*
	Qualification check.
*/
	if (NULL == m_db_tree) return 0 ;

	if (m_size < 1) {
		Left[0] = 1 ;
		path_len = 0 ;
		return 0 ;
		}

	Left[0] = j = m_root ;
	i = 1 ;

down_the_tree_12_04_94:

	// safety check : check if this node is in use.
	if (! m_db_tree[j].m_in_use) {
		// things are bad. the node we are comparing against is not in use; bomb this thing.
		throw 500 ;
		}

	if (m_db_tree[j].m_id == id) {
		Left[i] = 0 ;
		path_len = i ;
		return Left[i-1] ; // we found the entity
		}
	if (m_db_tree[j].m_id < id) { // go right
		if (m_db_tree[j].m_RC == 0) {
			Left[i] = 1 ;
			path_len = i ;
			return 0 ;
			}
		Left[i++] = j = m_db_tree[j].m_RC ;
		goto down_the_tree_12_04_94 ;
		}
	else {
		if (m_db_tree[j].m_LC == 0) {
			Left[i] = -1 ;
			path_len = i ;
			return 0 ;
			}
		Left[i++] = j = m_db_tree[j].m_LC ;
		goto down_the_tree_12_04_94 ;
		}
}


long CMauiAVLTreeSimple::FindNext(long key, long *next_key)  
{
	long j ;
	if ((j = Find(key))) {
		// this entity is already in the database; increase data.
		j = m_db_tree[j].m_next_entity ;
		if (0 == j) {
			if (next_key) *next_key = 0 ;
			}
		else {
			if (next_key) *next_key = m_db_tree[j].m_id ;
			}
		return 1 ;
		}

	return 0 ;
}


/*
	add an entity to the database
	return 0 if it fails (this entity is already in the database or no memory)
	or a positive number which is the index where to put this entity

	if this entity is already in the database don't return FAILURE, 
	return TRUE and replace the data associated with this key
*/

long CMauiAVLTreeSimple::Insert(long id)
{
#ifdef DEBUG_AVL_mutex
	if (NULL == _msAVLdebugMutex) 
		_msAVLdebugMutex = new COptexEXT_debug("AVLtestMutex") ;
	COptexEXTAutoLock lock(*_msAVLdebugMutex) ;
#endif // _DEBUG

	unsigned long i, j ;

/*
	Qualification check.
*/
	if (NULL == m_db_tree) return 0 ;

	if ((j = Find(id))) {
		// this entity is already in the database
		// don't return FAILURE, return TRUE and replace the data associated with this key
		return j ;
		}

#ifdef DEBUG_AVL_new
	if (! TestConsistency()) {
		int error = 999 ;
		}
	if (NULL == _msTestAVLTree) {
		try {
			_msTestAVLTree = new CMauiAVLTreeSimple ;
			}
		catch (...) {
			}
		}
	if (_msTestAVLTree) 
		(*_msTestAVLTree) << *this ;
#endif // DEBUG_AVL_new

	// find a place for this entity
	if (m_free_size < 1) {
		if (! ReAllocateMemory()) {
			return 0 ; // no space
			}
		}
	i = m_first_free_block ;
	m_first_free_block = m_db_tree[m_first_free_block].m_next_entity ;
	--m_free_size ;
	++m_size ;

	m_db_tree[i].m_LC = m_db_tree[i].m_RC = 0 ;
	m_db_tree[i].m_in_use = 1 ;
	m_db_tree[i].m_id = id ;
	m_db_tree[i].m_height = 0 ;
	m_db_tree[i].m_balance = 0 ;
	if (m_size < 2) {
		m_db_tree[i].m_prev_entity = m_db_tree[i].m_next_entity = 0 ;
		m_first_used_block = m_root = i ;
		return i ;
		}
	// this node is set up now, except for the prev and next links; and the 'first' member variable

	// update the parent
	if (Left[path_len] == 1) { // right son
		j = Left[--path_len] ; // j is the parent
		m_db_tree[j].m_RC = i ;

		// first, set up prev and next links
		// we want to add this node in its correct place in the middle of the list, 
		// so that the list is sorted
		// therefore : add this node after the parent
		m_db_tree[i].m_prev_entity = j ;
		if ((m_db_tree[i].m_next_entity = m_db_tree[j].m_next_entity)) 
			m_db_tree[m_db_tree[i].m_next_entity].m_prev_entity = i ;
		m_db_tree[j].m_next_entity = i ;

		if ((signed char) -1 == m_db_tree[j].m_balance) { // the parent is balanced now
			m_db_tree[j].m_balance = 0 ; // because it had (has) a left son
			return i ; // since the height did not change everything is fine
			}
		m_db_tree[j].m_balance = 1 ;
		m_db_tree[j].m_height = 1 ;
		}
	else { // left son
		j = Left[--path_len] ; // j is the parent
		m_db_tree[j].m_LC = i ;

		// NB! check if we need to update the 'first' member variable
		if (m_first_used_block == j) m_first_used_block = i ;

		// first, set up prev and next links
		// we want to add this node in its correct place in the middle of the list, 
		// so that the list is sorted
		// therefore : add this node before the parent
		m_db_tree[i].m_next_entity = j ;
		if ((m_db_tree[i].m_prev_entity = m_db_tree[j].m_prev_entity))
			m_db_tree[m_db_tree[i].m_prev_entity].m_next_entity = i ;
		m_db_tree[j].m_prev_entity = i ;

		if (1 == m_db_tree[j].m_balance) { // the parent is balanced now
			m_db_tree[j].m_balance = 0 ; // because it had (has) a right son
			return i ; // since the height did not change everything is fine
			}
		m_db_tree[j].m_balance = -1 ;
		m_db_tree[j].m_height = 1 ;
		}

	// the parent had no children, now it has one
	// ie. the height has increased by 1
	// the tree could potentially be unbalanced now
	// has to be balanced
	// Left[path_len] is the index of the current node ("the parent")
	Left[path_len + 1] = i ;
	BalanceInsertTree() ;

#ifdef DEBUG_AVL_new
	if (! TestConsistency()) {
//		(*this) << *_msTestAVLTree ;
//		Insert(id, data) ;
		int error = 999999 ;
		}
#endif // DEBUG_AVL_new

#ifdef DEBUG_AVL_new
	if (! Find(id, NULL)) {
//		(*this) << *_msTestAVLTree ;
//		Insert(id, data) ;
		int error = 999999 ;
		}
#endif // DEBUG_AVL_new

	return i ;
}


/*
	delete the first entity from the database
	return the index of the node if deleted, 0 if the id is not in the database
*/

long CMauiAVLTreeSimple::RemoveFirst(long *key /* output */) // returns TRUE iff everything is fine
{
#ifdef DEBUG_AVL_mutex
	if (NULL == _msAVLdebugMutex) 
		_msAVLdebugMutex = new COptexEXT_debug("AVLtestMutex") ;
	COptexEXTAutoLock lock(*_msAVLdebugMutex) ;
#endif // _DEBUG

	if (m_size < 1) 
		return 0 ;
	if (key) {
		*key = m_db_tree[m_first_used_block].m_id ;
		return Remove(*key) ;
		}
	return 1 ;
}


/*
	delete an entity from the database
	return the index of the node if deleted, 0 if the id is not in the database
*/

long CMauiAVLTreeSimple::Remove(long id)
{
#ifdef DEBUG_AVL_mutex
	if (NULL == _msAVLdebugMutex) 
		_msAVLdebugMutex = new COptexEXT_debug("AVLtestMutex") ;
	COptexEXTAutoLock lock(*_msAVLdebugMutex) ;
#endif // _DEBUG

	unsigned long i, j, k, r ;

/*
	Qualification check.
*/
	if (NULL == m_db_tree) return 0 ;

#ifdef DEBUG_AVL_new
	if (! TestConsistency()) {
		int error = 999 ;
		}
	if (NULL == _msTestAVLTree) {
		try {
			_msTestAVLTree = new CMauiAVLTreeSimple ;
			}
		catch (...) {
			}
		}
	if (_msTestAVLTree) 
		(*_msTestAVLTree) << *this ;
#endif // DEBUG_AVL_new

	if (! Find(id)) 
		return 0 ; // this entity is not in the database

	// take the node out of the used list and put in the free list
	j = Left[--path_len] ;
	// save data
	m_db_tree[0].m_in_use = m_db_tree[j].m_in_use ;
	m_db_tree[j].m_in_use = 0 ;

	// note that it is not a good idea to delete keys while we are traversing the
	// AVL tree using get_first() and get_next() functions.
	// however, we just try to do the best thing we can here : we move the current pointer back to the
	// previous entity so that when get_next() gets called next time, we move to the next one from here.
//	if (m_current) m_current = m_db_tree[j].m_prev_entity ;

	if (m_first_used_block == j) {
		m_first_used_block = m_db_tree[j].m_next_entity ;
		// NB! no previous entity
		if (m_first_used_block) m_db_tree[m_first_used_block].m_prev_entity = 0 ;
		}
	else {
		m_db_tree[m_db_tree[j].m_prev_entity].m_next_entity = m_db_tree[j].m_next_entity ;
		if (m_db_tree[j].m_next_entity) 
			m_db_tree[m_db_tree[j].m_next_entity].m_prev_entity = m_db_tree[j].m_prev_entity ;
		}

	m_db_tree[j].m_next_entity = m_first_free_block ;
	m_first_free_block = j ;
	--m_size ;
	++m_free_size ;

	// update the parent
	if (! m_db_tree[j].m_LC && ! m_db_tree[j].m_RC) { // a leaf
		if (path_len == 0) { // database empty
			m_size = 0 ;
			m_free_size = m_allocated_space ;
			m_first_used_block = 0 ;
			m_root = 0 ;
			return m_first_free_block ;
			}
		k = Left[--path_len] ;
		if (m_db_tree[k].m_LC == j) {
			m_db_tree[k].m_LC = 0 ;
			++(m_db_tree[k].m_balance) ;
			}
		else {
			m_db_tree[k].m_RC = 0 ;
			--(m_db_tree[k].m_balance) ;
			}
		if (m_db_tree[k].m_balance == 0) m_db_tree[k].m_height = 0 ;
		}
	else { // not a leaf. j is the index of the node to be deleted
		if (! m_db_tree[j].m_LC) { // no left child, only right child
			if (path_len == 0) { // it is the root
				m_root = m_db_tree[j].m_RC ;
				return m_first_free_block ;
				}
			k = Left[--path_len] ;
			if (m_db_tree[k].m_LC == j) {
				m_db_tree[k].m_LC = m_db_tree[j].m_RC ;
				++(m_db_tree[k].m_balance) ;
				}
			else {
				m_db_tree[k].m_RC = m_db_tree[j].m_RC ;
				--(m_db_tree[k].m_balance) ;
				}
			if (m_db_tree[k].m_balance == 0) m_db_tree[k].m_height-- ;
			}
		else if (! m_db_tree[j].m_RC){ // no right child, only left child
			if (path_len == 0) { // it is the root
				m_root = m_db_tree[j].m_LC ;
				return m_first_free_block ;
				}
			k = Left[--path_len] ;
			if (m_db_tree[k].m_LC == j) {
				m_db_tree[k].m_LC = m_db_tree[j].m_LC ;
				++m_db_tree[k].m_balance ;
				}
			else {
				m_db_tree[k].m_RC = m_db_tree[j].m_LC ;
				--m_db_tree[k].m_balance ;
				}
			if (m_db_tree[k].m_balance == 0) m_db_tree[k].m_height-- ;
			}
		else { // has both children
			// find inorder successor
			k = m_db_tree[j].m_RC ;
			i = path_len ;
find_inorder_successor_12_07_1994:
			Left[++path_len] = k ;
			if (m_db_tree[k].m_LC) k = m_db_tree[k].m_LC ;
			else goto switch_nodes_12_07_1994 ;
			goto find_inorder_successor_12_07_1994 ;

switch_nodes_12_07_1994:
			r = m_db_tree[k].m_RC ;
			m_db_tree[k].m_balance = m_db_tree[j].m_balance ;
			m_db_tree[k].m_height = m_db_tree[j].m_height ;
			m_db_tree[k].m_LC = m_db_tree[j].m_LC ;
			m_db_tree[k].m_RC = m_db_tree[j].m_RC ;
			if (i == 0) m_root = k ;
			else {
				if (m_db_tree[Left[i-1]].m_LC == j) m_db_tree[Left[i-1]].m_LC = k ;
				else m_db_tree[Left[i-1]].m_RC = k ;
				}
			Left[i] = k ;

			j = Left[--path_len] ;
			if (m_db_tree[j].m_LC == k) {
				m_db_tree[j].m_LC = r ;
				++m_db_tree[j].m_balance ;
				}
			else { // this is only possible if this node is the right son of the original node to be deleted
				m_db_tree[j].m_RC = r ;
				--m_db_tree[j].m_balance ;
				}
			if (m_db_tree[j].m_balance == 0) m_db_tree[j].m_height-- ;
			k = j ;
			}
		}

	if (m_db_tree[k].m_balance == 1 || m_db_tree[k].m_balance == (signed char) -1) return m_first_free_block ;
	// everything is fine, the height did not change

	BalanceDeleteTree() ;

	return m_first_free_block ;
}


/*
	this function does not move the current pointer
*/

//long CMauiAVLTreeSimple::get_first_key_data(long *key /* can be NULL */, long *data /* can be NULL */)
//{
//	if (m_size < 1) return 0 ;
//	if (key) *key = m_db_tree[m_first_used_block].m_id ;
//	if (data) *data = m_db_tree[m_first_used_block].m_data ;
//	return 1 ;
//}


/*
	this function does not move the current pointer
*/

//long CMauiAVLTreeSimple::get_second_key_data(long *key /* can be NULL */, long *data /* can be NULL */)
//{
//	if (m_size < 2) return 0 ;
//	long next_ent = m_db_tree[m_first_used_block].m_next_entity ;
//	if (key) *key = m_db_tree[next_ent].m_id ;
//	if (data) *data = m_db_tree[next_ent].m_data ;
//	return 1 ;
//}


/*
	To get the smallest key and its data in the tree.
	Returns FALSE iff the tree is empty or no next element.
	Sets the selection/current pointer to the first node in the tree.
*/

long CMauiAVLTreeSimple::GetFirst(long *key /* can be NULL */, long & current) const 
{
	if (m_size < 1) {
		current = 0 ;
		return 0 ;
		}

	current = m_first_used_block ;
	if (key) *key = m_db_tree[current].m_id ;
	return 1 ;
}


/*
	Member functions to traverse the tree.
	Returns FALSE iff the tree is empty or no next element.
	Moves the current selection pointer to the next node in the tree.
*/

long CMauiAVLTreeSimple::GetNext(long *key /* can be NULL */, long & current) const 
{
	// if the tree is empty, cannot select anything
	if (m_size < 1) {
		if (key) *key = 0 ;
		current = 0 ;
		return 0 ;
		}

	// check if the current pointer has passed the last node.
	// if current pointer is 0, then the last node returned was the last node in the tree.
	if (current < 0) {
		current = m_first_used_block ;
		if (key) *key = m_db_tree[current].m_id ;
		return 1 ;
		}
	if (0 == current || current > (long)m_allocated_space) {
		if (key) *key = 0 ;
		current = 0 ;
		return 0 ;
		}

	// check whether the current pointer is valid, and points to a valid node (ie. node in use).
	if (! m_db_tree[current].m_in_use) {
		// this means that, since there are keys left in the AVL tree,
		// we have lost track of the current key.
		// most likely we have deleted the current key.
		// take the first one.
		current = m_first_used_block ;
		if (key) *key = m_db_tree[current].m_id ;
		return 1 ;
		}

	// get next key
	current = m_db_tree[current].m_next_entity ;
	// check if points to valid node
	if (0 == current || current > (long)m_allocated_space) {
		if (key) *key = 0 ;
		return 0 ;
		}

	if (key) *key = m_db_tree[current].m_id ;

	return 1 ;
}


// this function returns the key/data pointed to by 'current' 
// (if -1==current, then this means the first, if 0==current, then this means NONE).
// it also forwars the 'current' pointer to the next element in the set.
long CMauiAVLTreeSimple::GetCurrentAndAdvance(long *key /* can be NULL */, long & current) const 
{
	// if the tree is empty, cannot select anything
	if (m_size < 1) {
		if (key) *key = 0 ;
		current = 0 ;
		return 0 ;
		}

	// check if the current pointer has passed the last node.
	// if current pointer is 0, then the last node returned was the last node in the tree.
	if (current < 0) {
		current = m_first_used_block ;
		}
	if (0 == current || current > (long)m_allocated_space) {
		if (key) *key = 0 ;
		current = 0 ;
		return 0 ;
		}

	// check whether the current pointer is valid, and points to a valid node (ie. node in use).
	if (! m_db_tree[current].m_in_use) {
		// this means that, since there are keys left in the AVL tree,
		// we have lost track of the current key.
		// most likely we have deleted the current key.
		// take the first one.
		current = m_first_used_block ;
		}

	if (key) *key = m_db_tree[current].m_id ;

	// get next key
	current = m_db_tree[current].m_next_entity ;

	return 1 ;
}


/*
	Idea : at every point in time, we are checking if a given key is in the tree, by traversing
	the tree from left to right. We start with 1. Every time we find the key, we increment the
	key we are looking for by 1 and keep looking.
*/

long CMauiAVLTreeSimple::FindSmallestPositiveKeyNotUsed(void)
{
	long i, j, target_key ;

	// check if the tree is empty
	if (m_size < 1) return 1 ;

	// start with 1
	target_key = 1 ;

	// we will use 'Middle' to keep the stack (actually, the path from the root to the current node in the tree)
	Middle[0] = j = m_root ;
	// 'Left' will be used to keep track which of the children (none, left, right) of the current node
	// we have already traversed
	Left[0] = -1 ;
	i = 0 ;

down_the_tree_10_11_95 :

	if (m_db_tree[j].m_id > target_key) {
		// ok, we are done. we found that the target_key is not in the tree.
		return target_key ;
		}
	// else we have to keep looking

	if (m_db_tree[j].m_id == target_key) target_key++ ;

backtrack_10_11_95 :

	// see if we can go left
	if (Left[i] < 0 && m_db_tree[j].m_LC > 0) {
		Left[i] = 0 ;
		Middle[++i] = j = m_db_tree[j].m_LC ;
		Left[i] = -1 ;
		goto down_the_tree_10_11_95 ;
		}

	// ok, cannot go left. see if we can go right.
	if (Left <= 0 && m_db_tree[j].m_RC > 0) {
		Left[i] = 1 ;
		Middle[++i] = j = m_db_tree[j].m_RC ;
		Left[i] = -1 ;
		goto down_the_tree_10_11_95 ;
		}

	// ok, cannot also go right. see if we can backtrack
	if (--i >= 0) goto backtrack_10_11_95 ;

/*
	If we get to this point, it means that we have exhausted the tree, but target_key was not there.
*/
	return target_key ;
}


long CMauiAVLTreeSimple::TestTree(int numIterations, char **pErrorStr) // returns TRUE iff the tree is OK.
{
	if (pErrorStr) 
		*pErrorStr = NULL ;
	if (numIterations < 1) 
		return 1 ;

	// we will need to keep track of the data we insert into the tree; we will use this array.
#define MAX_TEST_KEYS 512
	long keys[MAX_TEST_KEYS] ;
	int nk = 0 ;

	// we will execute a number of sequences of operations: insert, remove.
	int n = 0 ;
	unsigned long minSize = 999999, maxSize = 0 ;
	int totalSize = 0, totalN = 0 ;
	Empty() ;
	while (n < numIterations) {
		// collect statistics
		totalSize += GetSize() ;
		++totalN ;
		if (minSize > GetSize()) minSize = GetSize() ;
		if (maxSize < GetSize()) maxSize = GetSize() ;

		// compute sequence size
		int l = mtrandgen.randInt(99) ;
		if (l < 50) l = 1 + mtrandgen.randInt(2) ; // 50% of the time
		else { // rest of the time
			l = mtrandgen.randInt(24) ;
			l = 1 + l*l ;
			}

		// decide whether to do an insertion or deletion
		int op = 0 ;
		if (0 == nk) op = 1 ; // insertion
		else if (nk > 256) op = 0 ; // deletion
		else op = mtrandgen.randInt(1) ;

		// do the operations
		int i, j ;
		if (0 == op) {
			for (i = 0 ; i < l && n < numIterations ; i++) {
				if (0 == nk) break ; // check if out of keys
				// with a small probability, try to remove a non-existent key)
				long key = -1 ;
				int p = mtrandgen.randInt(19) ;
				if (0 == p) {
					key = mtrandgen.randInt(999999) ;
					}
				else {
					// pick a key from the data set
					int k = mtrandgen.randInt(nk-1) ;
					key = keys[k] ;
					--nk ;
					keys[k] = keys[nk] ;
					}

				Remove(key) ;
				++n ; // count operations
				if (! CheckTree(pErrorStr)) {
					return 0 ;
					}
				}
			}
		else {
			for (i = 0 ; i < l && n < numIterations ; i++) {
				if (nk >= MAX_TEST_KEYS) 
					break ;
				// pick a key from the data set
				int key = mtrandgen.randInt(999999) ;
				// check if key already exists
				for (j = 0 ; j < nk ; j++) {
					if (keys[j] == key) break ;
					}
				// if key is new, store it
				if (j >= nk) {
					keys[nk++] = key ;
					}

				Insert(key) ;
				++n ; // count operations
				if (! CheckTree(pErrorStr)) {
					return 0 ;
					}
				}
			}
		}
	Empty() ;

	return 1 ;
}


bool CMauiAVLTreeSimple::operator<<(const CMauiAVLTreeSimple & AVLtree)
{
	unsigned long i ;

	// if current memory not enough, let it go
	if (NULL == m_db_tree || m_allocated_space < AVLtree.m_allocated_space) {
		if (m_db_tree) {
			delete[] m_db_tree ;
			m_db_tree = NULL ;
			m_allocated_space = 0 ;
			}
		if (! AllocateMemory(AVLtree.m_allocated_space + 1)) 
			return false ;

		m_allocated_space = AVLtree.m_allocated_space ;
		m_size = AVLtree.m_size ;
		m_free_size = AVLtree.m_free_size ;
		m_first_free_block = AVLtree.m_first_free_block ;
		m_first_used_block = AVLtree.m_first_used_block ;
		m_root = AVLtree.m_root ;
		m_re_allocation_size = AVLtree.m_re_allocation_size ;
		for (i = 0 ; i <= AVLtree.m_allocated_space ; i++) 
			m_db_tree[i] << AVLtree.m_db_tree[i] ;
		}
	else {
		// reuse existing memory
		m_size = AVLtree.m_size ;
		m_free_size = AVLtree.m_free_size ;
		m_first_free_block = AVLtree.m_first_free_block ;
		m_first_used_block = AVLtree.m_first_used_block ;
		m_root = AVLtree.m_root ;
		m_re_allocation_size = AVLtree.m_re_allocation_size ;
		for (i = 0 ; i <= AVLtree.m_allocated_space ; i++) 
			m_db_tree[i] << AVLtree.m_db_tree[i] ;
		// the rest of the space is empty
		if (m_allocated_space > AVLtree.m_allocated_space) {
			m_db_tree[i].m_in_use = 0 ;
			for (++i ; i <= m_allocated_space ; i++) {
				m_db_tree[i-1].m_next_entity = i ;
				m_db_tree[i].m_in_use = 0 ;
				}
			m_db_tree[m_allocated_space].m_next_entity = m_first_free_block ;
			m_first_free_block = AVLtree.m_allocated_space + 1 ;
			m_free_size += m_allocated_space - AVLtree.m_allocated_space ;
			}
		}

	return true ;
}


bool CMauiAVLTreeSimple::TestConsistency(long checkbitvector) const
{
	// checkbitvector & 1 = check height
	// checkbitvector & 2 = check balance

	if (NULL == m_db_tree) return true ;
	unsigned long i, n ;

	// NOTE : all indeces must be within [1, m_allocated_space]

	// BASIC CHECKS
	if (-1 != m_db_tree[0].m_height) 
		return false ;

	// count used/free nodes; check actual counts match what is known 
	n = 0 ;
	for (i = m_first_used_block ; i ; i = m_db_tree[i].m_next_entity, n++) {
		if (m_db_tree[i].m_next_entity > m_allocated_space) {
			// m_next_entity is out of bounds
			return false ;
			}
		if (! m_db_tree[i].m_in_use) 
			// node that should be in use is marked as not-in-use
			return false ;
		}
	if (n != m_size) 
		// actual number of used nodes differs from what is known
		return false ;
	n = 0 ;
	for (i = m_first_free_block ; i ; i = m_db_tree[i].m_next_entity, n++) {
		if (m_db_tree[i].m_next_entity > m_allocated_space) {
			// m_next_entity is out of bounds
			return false ;
			}
		if (m_db_tree[i].m_in_use) 
			// node that should be not in use is marked as in-use
			return false ;
		}
	if (n != m_free_size) 
		// actual number of free nodes differs from what is known
		return false ;

	// CHECK BINARY TREE PROPERTY: left < this < right
	// go through all used nodes; check that left child is smaller, right child is larger.
	for (i = m_first_used_block ; i ; i = m_db_tree[i].m_next_entity) {
		if (m_db_tree[i].m_LC) {
			unsigned long lid = m_db_tree[i].m_LC ;
			if (m_db_tree[lid].m_id > m_db_tree[i].m_id) 
				return false ;
			}
		if (m_db_tree[i].m_RC) {
			unsigned long rid = m_db_tree[i].m_RC ;
			if (m_db_tree[rid].m_id < m_db_tree[i].m_id) 
				return false ;
			}
		}

	// CHECK HEIGHT OF EACH NODE
	if (checkbitvector & 1) {
		for (i = m_first_used_block ; i ; i = m_db_tree[i].m_next_entity) {
			signed short h = 0 ;
			if (m_db_tree[i].m_LC) {
				signed short lh = m_db_tree[m_db_tree[i].m_LC].m_height + 1 ;
				if (h < lh) 
					h = lh ;
				}
			if (m_db_tree[i].m_RC) {
				signed short rh = m_db_tree[m_db_tree[i].m_RC].m_height + 1 ;
				if (h < rh) 
					h = rh ;
				}
			if (m_db_tree[i].m_height != h) {
				return false ;
				}
			}
		}

	// CHECK BALANCE OF EACH NODE
	if (checkbitvector & 2) {
		for (i = m_first_used_block ; i ; i = m_db_tree[i].m_next_entity) {
			signed short lh = -1 ;
			signed short rh = -1 ;
			if (m_db_tree[i].m_LC) {
				lh = m_db_tree[m_db_tree[i].m_LC].m_height ;
				}
			if (m_db_tree[i].m_RC) {
				rh = m_db_tree[m_db_tree[i].m_RC].m_height ;
				}
			signed char balance = rh - lh ;
			if (balance != m_db_tree[i].m_balance) 
				return false ;
			}
		}

	return true ;
}

