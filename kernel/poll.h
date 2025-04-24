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

	struct fd_set {
		int set[100];
	};

	const int FD_SETSIZE = 1024; // FD_SETSIZE is the maximum number of FDs, standard is 1024
	// fd_set is a bitmask for file descriptors
	typedef struct {
		// each bit represents a FD
		uint8_t fds[FD_SETSIZE / 8]; 
	} fd_set;
	
	// helper functions for manipulating fd_set
	void FD_ZERO(fd_set *set) {
		for (int i = 0; i < FD_SETSIZE / 8; i++) {
			set->fds[i] = 0;
		}
	}

 	bool FD_SET(int fd, fd_set *set) {
		if (fd > 1023) return false; // hard limit for # of fds
		int index = fd / 8; // find index of array
		int mask = 1 << ((fd) % 8); // which bit in index? 
		set->fds[index] |= mask; // flipping bit to 1
		return true;
	}

	bool FD_ISSET(int fd, fd_set *set) {
		if (fd > 1023) return false; 
		int index = fd / 8; 
		int mask = 1 << ((fd) % 8); 
		return set->fds[index] & mask; // 0 == not set, 1 == set
	}
	
	int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timespec *timeout, PCB* cur_pcb) {
		if (nfds < 0) return -1;
	
		Pollfd poll_fds[FD_SETSIZE];
		int poll_fd_count = 0;
	
		// Translate fd_set to Pollfd array
		for (int fd = 0; fd < nfds; fd++) {
			short events = 0;
			if (readfds && FD_ISSET(fd, readfds)) events |= PE_READ; // read?
			if (writefds && FD_ISSET(fd, writefds)) events |= PE_WRITE; // write?
			if (exceptfds && FD_ISSET(fd, exceptfds)) events |= PE_ERR; // Exceptional conditions
			if (events != 0) {
				poll_fds[poll_fd_count] = {fd, events, 0};
				poll_fd_count++;
			}
		}
	
		// Convert timeval to timeout in milliseconds
		int timeout_ms;
		if (!timeout) {
			timeout_ms = -1; // Wait indefinitely
		} else {
			timeout_ms = timeout->tv_sec * 1000 + timeout->tv_nsec / 1000000; // Convert to ms
		}
	
		// Call poll
		int ready_fds = poll(poll_fds, poll_fd_count, timeout_ms, cur_pcb);
	
		// If poll failed, return error
		if (ready_fds < 0) return -1;
	
		// Clear fd_sets
		if (readfds) FD_ZERO(readfds);
		if (writefds) FD_ZERO(writefds);
		if (exceptfds) FD_ZERO(exceptfds);

		// Update fd_sets based on poll results
		for (int j = 0; j < poll_fd_count; j++) {
			Pollfd &pfd = poll_fds[j];
			if (pfd.revents & PE_READ && readfds) {
				FD_SET(pfd.fd, readfds);
			}
			if (pfd.revents & PE_WRITE && writefds) {
				FD_SET(pfd.fd, writefds);
			}
			if (pfd.revents & PE_ERR && exceptfds) {
				FD_SET(pfd.fd, exceptfds);
			}
		}
		return ready_fds;
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
	//so it will be ignored for now (may cause issues, not sure)
	int ppoll(Pollfd* fds, int nfds, const timespec* tmo_p, const sigset_t* sigmask, PCB* pcb) {
		//poll but with increased time precision, and sigmask
		//no tmo_p means wait indefinitely, stay looping until all files are ready
	
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
	
		return poll(fds, nfds, timeout_in_ms, pcb);
	}
	
}
