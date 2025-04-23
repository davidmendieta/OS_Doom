#pragma once

#include <stdint.h>
#include "ext2.h"
#include "pit.h"
#include "processes.h"
#include "threads.h"

namespace Poll {

	//got these numbers from chatgpt
	enum PollEvent : uint16_t {
		PE_NONE   = 0,
		PE_READ   = 0x0001,  // POLLIN
		PE_WRITE  = 0x0004,  // POLLOUT
		PE_ERR    = 0x0008,  // POLLERR
		PE_HUP    = 0x0010,  // POLLHUP
		PE_INVAL  = 0x0020   // POLLNVAL
	};

	//struct from https://manpages.debian.org/unstable/manpages-dev/poll.2.en.html
	struct Pollfd {
		int fd;       		  //file descriptor
		short events;         //requested events 
		short revents;        //returned events
	};

	//struct from google ai overview ("what is a timespec")
	struct timespec {
		int tv_sec;         //Seconds 
		long int tv_nsec;   //Nanoseconds [0, 999,999,999] 
	};

	//empty struct used to avoid errors in ppoll function
	struct sigset_t {};

	//going based off of node (file perms), may be incorrect
	//checks the files user, group, and other permissions 
	bool can_read (StrongPtr<OpenFile> fd) {
		auto mode = fd->node->data.mode;
		return mode & (0x100 | 0x040 | 0x004); 
	}
	
	bool can_write (StrongPtr<OpenFile> fd) {
		auto mode = fd->node->data.mode;
		return mode & (0x080 | 0x020 | 0x002);
	}
	

	bool read (uint16_t events) {
		return events & PE_READ;
	}

	bool write (uint16_t events) {
		return events & PE_WRITE;
	}

	//user stub is just: 
	//int poll (Pollfd* fds, int nfds, int timeout);

	int poll(Pollfd* fds, int nfds, int timeout, PCB* cur_pcb) {
		if (nfds < 0) {
			return -1;
		}
	
		uint32_t fd_ready = 0;
		uint32_t start_time = Pit::jiffies;
		uint32_t cur_time = Pit::jiffies;

		//per POSIX
		//timeout > 0 - wait up to some amt of milliseconds
		//timeout == 0 - check only once and return 
		//timeout < 0 - check indefinitely (no timeout) until all fds are ready
		bool infinite = timeout < 0;
		bool one_time = timeout == 0;
		bool timed_out = false;
	
		while (!timed_out && fd_ready < (uint32_t)nfds) {
			for (uint32_t fd_index = 0; fd_index < (uint32_t)nfds; fd_index++) {
				Pollfd& cur = fds[fd_index];
	
				if (cur.revents != 0) {
					continue;
				}
	
				StrongPtr<OpenFile> cur_file = cur_pcb->fdt[cur.fd];
				if (cur_file == nullptr || cur_file->node == nullptr) {
					cur.revents |= PE_INVAL;
					fd_ready++;
					continue;
				}
	
				bool file_read = can_read(cur_file);
				bool file_write = can_write(cur_file);
	
				bool event_read = read(cur.events);
				bool event_write = write(cur.events);
	
				short prev_revents = cur.revents;
	
				if (file_read && event_read) {
					cur.revents |= PE_READ;
				}
				if (file_write && event_write) {
					cur.revents |= PE_WRITE;
				}
	
				if (cur.revents != 0 && prev_revents == 0) {
					fd_ready++;
				}
			}
	
			// exit if one-time or fds became ready
			if (one_time || fd_ready > 0) {
				break;
			}
	
			cur_time = Pit::jiffies;
	
			if (!infinite && (cur_time >= start_time + (uint32_t)timeout)) {
				timed_out = true;
			}
		}
	
		// if none ready and timed out, return 0
		if (fd_ready == 0 && timed_out) {
			return 0;
		}
	
		return fd_ready;
	}
	

	//user stub is
	//int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *_Nullable tmo_p,
	//const sigset_t *_Nullable sigmask);

	//sigmask represents signals from the OS (i think), i dont think this is implemented
	//so it will be ignored (may cause issues, not sure)
	int ppoll(Pollfd* fds, int nfds, const timespec* tmo_p, const sigset_t* sigmask, PCB* pcb) {
		//poll but with increased time precision, and sigmask
		//no tmo_p means wait indefinitely, stay looping untill all files are ready
	
		//early return if invalid nfds
		if (nfds < 0) {
			return -1;
		}
	
		int timeout_in_ms;
	
		//determine what to set the timeout time to
		if (tmo_p == nullptr) {
			//no time spec given, means wait indefinitely
			timeout_in_ms = -1;
		} else {
			//given a timespec
			//convert timespec into milliseconds to reuse poll
			timeout_in_ms = tmo_p->tv_sec * 1000; //convert seconds to milliseconds
			//convert nanoseconds into milliseconds by rounding up to the nearest millisecond
			timeout_in_ms += (tmo_p->tv_nsec + 999999) / 1000000; 
		}
	
		// Call your existing poll implementation
		return poll(fds, nfds, timeout_in_ms, pcb);
	}
	
}
