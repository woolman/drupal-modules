Woolman Semester Module
=======================

What it Does
------------

* Front End - self-service enrollment
    * Student begin their application (opens a case) at [semester.woolman.org/admissions/apply](http://semester.woolman.org/admissions/apply)
    * Application forms can be completed or saved as a draft
* Back End - staff [view and manage applications](https://woolman.org/staff/admissions/applications)
    * Allows all open cases to be managed from a single page (creating/editing activities, changing status, etc)
    * Email notifications to staff for all case activity
    * Staff can manage [application form questions](http://semester.woolman.org/admissions/apply/settings)

How it Works
------------

* Application process is a CiviCase type
* All forms are hard-coded with some configuration options (webform_civicrm would be a good substitute)
* Backend staff screen written with drupal ajax & jquery

General Notes
------------

This module  uses Drupal for the UI, and CiviCRM for storing and retrieving data. This is done using CiviCRM APIs where possible, although some direct SQL queries on the woolman_civicrm database were unavoidable (they can also be vastly more efficient when trying to retrieve large amounts of data such as building reports).

This module is designed to compliment CiviCRM rather than replace it. It does not store any CiviCRM data in a non-standard way. So if you were to change any camp registration data in CiviCRM (manually registering a camper, for example), this module will not be "confused" by being "overridden." As always, it will just accept and work with the data in the CiviCRM DB.

This module has some additional functionality for tracking prospective students which is no longer used by current staff. The "prospective student" case type, plus the custom code to support it, could be removed.
