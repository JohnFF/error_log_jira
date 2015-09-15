README.txt
==========

A module that exports the watchdog log messages from the watchdog table
and inserts them in jira, as issues.

The user navigates on admin/config/services/jira_rest, where he completes the
form fields with the jira username, password and URL.

Then in admin/config/development/logging/jira, chooses what message type jira
will send after cron runs and how many will check. Its important, also to fill
the project KEY (not the TITLE). When the cron runs, the module will check if
the log message exists. If it exists, with resolution as "Unresolved", the
module doesn't creates the issue or if exists, with resolution as "Resolved",
then creates again this issue as "Unresolved" in jira.
