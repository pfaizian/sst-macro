/**
Copyright 2009-2018 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2018, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#ifndef SSTMAC_SOFTWARE_THREADING_THREADING_INTERFACE_H_INCLUDED
#define SSTMAC_SOFTWARE_THREADING_THREADING_INTERFACE_H_INCLUDED

#include <sstmac/common/sstmac_config.h>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <sprockit/factories/factory.h>

namespace sstmac {
namespace sw {


/**
 * @brief The thread_context class
 */
class thread_context
{
 public:
  DeclareFactory(thread_context)

  virtual ~thread_context() {}

  virtual thread_context* copy() const = 0;

  virtual void init_context() = 0;

  virtual void destroy_context() = 0;

  /**
   * @brief start_context
   * @param physical_thread_id  An optional ID for
   * @param stack
   * @param stacksize
   * @param args
   * @param globals_storage
   * @param from
   */
  virtual void start_context(int physical_thread_id,
                void *stack, size_t stacksize,
                void (*func)(void*), void *args,
                void* globals_storage, void* tls_storage,
                thread_context* from) = 0;

  virtual void resume_context(thread_context* from) = 0;

  virtual void pause_context(thread_context* to) = 0;

  /**
   * @brief jump_context
   * Jump directly from one context to another.
   * This bypasses the safety of the start/pause/resume pattern
   * @param to
   */
  virtual void jump_context(thread_context* to){
    to->resume_context(this);
  }

  /**
   * @brief complete_context Perform all cleanup operations to end this context
   * @param to
   */
  virtual void complete_context(thread_context* to) = 0;

  static std::string default_threading();

 protected:
  thread_context() {}

};
}
} // end of namespace sstmac
#endif
