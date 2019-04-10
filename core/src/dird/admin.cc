/*
   BAREOS® - Backup Archiving REcovery Open Sourced

   Copyright (C) 2003-2012 Free Software Foundation Europe e.V.
   Copyright (C) 2016-2019 Bareos GmbH & Co. KG

   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version three of the GNU Affero General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.
*/
/*
 * Kern Sibbald, May MMIII
 */

/**
 * @file
 *
 * responsible for doing admin jobs
 */

#include "include/bareos.h"
#include "dird.h"
#include "dird/admin.h"
#include "dird/job.h"
#include "dird/storage.h"

namespace directordaemon {

static const int debuglevel = 100;

bool DoAdminInit(JobControlRecord* jcr)
{
  FreeRstorage(jcr);
  if (!AllowDuplicateJob(jcr)) { return false; }

  return true;
}

/**
 * Returns: false on failure
 *          true  on success
 */
bool do_admin(JobControlRecord* jcr)
{
  jcr->jr.JobId = jcr->JobId;

  jcr->fname = (char*)GetPoolMemory(PM_FNAME);

  /*
   * Print Job Start message
   */
  Jmsg(jcr, M_INFO, 0, _("Start Admin JobId %d, Job=%s\n"), jcr->JobId,
       jcr->Job);

  jcr->setJobStatus(JS_Running);
  AdminCleanup(jcr, JS_Terminated);

  return true;
}

/**
 * Release resources allocated during backup.
 */
void AdminCleanup(JobControlRecord* jcr, int TermCode)
{
  char sdt[50], edt[50], schedt[50];
  char term_code[100];
  const char* TermMsg;
  int msg_type;

  Dmsg0(debuglevel, "Enter AdminCleanup()\n");

  UpdateJobEnd(jcr, TermCode);

  if (!jcr->db->GetJobRecord(jcr, &jcr->jr)) {
    Jmsg(jcr, M_WARNING, 0,
         _("Error getting Job record for Job report: ERR=%s"),
         jcr->db->strerror());
    jcr->setJobStatus(JS_ErrorTerminated);
  }

  msg_type = M_INFO; /* by default INFO message */
  switch (jcr->JobStatus) {
    case JS_Terminated:
      TermMsg = _("Admin OK");
      break;
    case JS_FatalError:
    case JS_ErrorTerminated:
      TermMsg = _("*** Admin Error ***");
      msg_type = M_ERROR; /* Generate error message */
      break;
    case JS_Canceled:
      TermMsg = _("Admin Canceled");
      break;
    default:
      TermMsg = term_code;
      sprintf(term_code, _("Inappropriate term code: %c\n"), jcr->JobStatus);
      break;
  }
  bstrftimes(schedt, sizeof(schedt), jcr->jr.SchedTime);
  bstrftimes(sdt, sizeof(sdt), jcr->jr.StartTime);
  bstrftimes(edt, sizeof(edt), jcr->jr.EndTime);

  Jmsg(jcr, msg_type, 0,
       _("BAREOS " VERSION " (" LSMDATE "): %s\n"
         "  JobId:                  %d\n"
         "  Job:                    %s\n"
         "  Scheduled time:         %s\n"
         "  Start time:             %s\n"
         "  End time:               %s\n"
         "  Bareos binary info:     %s\n"
         "  Termination:            %s\n\n"),
       edt, jcr->jr.JobId, jcr->jr.Job, schedt, sdt, edt, BAREOS_JOBLOG_MESSAGE,
       TermMsg);

  Dmsg0(debuglevel, "Leave AdminCleanup()\n");
}

} /* namespace directordaemon */
