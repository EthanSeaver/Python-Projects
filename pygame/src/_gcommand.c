/*
  pygame - Python Game Library

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * _movie - movie support for pygame with ffmpeg
 * Author: Tyler Laing
 *
 * This module allows for the loading of, playing, pausing, stopping, and so on
 *  of a video file. Any format supported by ffmpeg is supported by this
 *  video player. Any bugs, please email trinioler@gmail.com :)
 */

#include "_gcommand.h"

/* command queue management routines... I make a lot of queues, don't I? */
void addCommand(CommandQueue *q, Command *comm)
{
    SDL_LockMutex(q->q_mutex);
    comm->next=NULL;
    q->registry[comm->type]++;
    if(!q->size)
    {
        q->first=comm;
        q->size++;
        SDL_UnlockMutex(q->q_mutex);
        return;
    }
    if(q->size==1)
    {
        q->last=comm;
        q->first->next=comm;
        q->size++;
        SDL_UnlockMutex(q->q_mutex);
        return;
    }
    q->last->next=comm;
    q->last=comm;
    q->size++;
    SDL_UnlockMutex(q->q_mutex);
    return;
}

Command *getCommand(CommandQueue *q)
{
    SDL_LockMutex(q->q_mutex);
    Command *comm;
    if(!q->last && q->first)
    {
        comm=q->first;
        q->size--;
        SDL_UnlockMutex(q->q_mutex);
        return comm;
    }
    else if (!q->last && !q->first)
    {
        SDL_UnlockMutex(q->q_mutex);
        return NULL;
    }
    comm=q->first;
    q->first=q->first->next;
    q->size--;
    SDL_UnlockMutex(q->q_mutex);
    return comm;
}

inline int hasCommand(CommandQueue *q)
{
    if(q->size>0)
        return 1;
    return 0;
}

void flushCommands(CommandQueue *q)
{
    SDL_LockMutex(q->q_mutex);
    Command *prev;
    Command *cur = q->first;
    while(cur!=NULL)
    {
        prev=cur;
        cur=cur->next;
        PyMem_Free(prev);
        q->size--;
    }
    SDL_UnlockMutex(q->q_mutex);
}

/* registers a command with a particular movie object's command queue.
 *  Basically, this means, theoretically, different movie objects could have different commands...
 */
int registerCommand(CommandQueue *q)
{
    //int cur = q->reg_ix;
    if(q->reg_ix>=1024)
        q->reg_ix=0;
    q->registry[q->reg_ix]=0;
    q->reg_ix++;
    return q->reg_ix-1;
}
