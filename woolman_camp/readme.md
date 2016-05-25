Woolman Camp Registration Module
================================

What it Does
------------

* Front End - self-service enrollment
    * Lists sessions and availability at [camp.woolman.org/register](http://camp.woolman.org/register)
    * Enrollment forms at [camp.woolman.org/register/form](http://camp.woolman.org/register/form)
    * Allows online signup for shuttle rides and filling medical forms
    * Online payments via customized [CiviContribute page](http://woolman.org/civicrm/contribute/transact?id=7) accepts partial payments
* Back End - staff [view and manage camp enrollment](https://woolman.org/staff/camp/registration)
    * Displays graphs of registration levels by year and by session
    * Workflow for staff to review & approve registrations
    * Staff can give arbitrary discounts and override age restrictions
    * Staff can record payments and attendance
    * Provides a bookeeping report
    * Staff can manage [camp dates & rates](https://woolman.org/staff/camp/rates).

How it Works
------------

* Camps and shuttles are CiviEvent Types
* Camp sessions and shuttle trips are CiviEvents
* Fees are calculated using custom code, not Civi price sets
* Campers and parents are Civi Contacts
* Families are tracked via Civi relationships (we don't use Households)
* Enrollment forms are hard-coded with drupal FAPI and push Civi data using the api
* Medical form is an activity
* Staff graphs are generated using an old php library - could update this to something modern & javascripty
* Uses hooks to alter [CiviContribute page](http://woolman.org/civicrm/contribute/transact?id=7) and inject camp registration details (amount owed, etc)


General Notes
------------

This module has a lot in common with the Woolman Semester Online Application system in that it uses Drupal for the UI, and CiviCRM for storing and retrieving data. This is done using CiviCRM APIs where possible, although some direct SQL queries on the woolman_civicrm database were unavoidable (they can also be vastly more efficient when trying to retrieve large amounts of data such as building reports).

This module does have three tables in the drupal db: `woolman_camp_discounts`, `woolman_camp_age_exception` and `woolman_camp_rates`. The first is for storing scholarships and other discounts given to individual families (keyed to the parent who did the registering). The second allows a camper to appear older or younger to the system, allowing staff to override the behavior that restricts certain age campers from attending certain sessions. The last is for storing general info about each year's rates and discounts, used for calculating fees.

This module is designed to compliment CiviCRM rather than replace it. It does not store any CiviCRM data in a non-standard way. So if you were to change any camp registration data in CiviCRM (manually registering a camper, for example), this module will not be "confused" by being "overridden." As always, it will just accept and work with the data in the CiviCRM DB.

*Big Exception:* CiviEvent has no built-in way of allowing a person to register another person for an event without also registering themselves. (i.e. a parent registering their kid - obviously the parent isn't also coming to camp!) So we get around this pretty effectively with a custom participant field - "registered by". Also, Civi doesn't allow multiple payments per event, or multiple events per payment! So we get around that by only loosly connecting payments with participants (the only connection is the year and the contact who did the registering, which works fine). We store the year in the "source" column of the contribution.

Understanding and respecting the workflow this module follows is important for keeping a clean, accurate database.

Registration Workflow
---------------------

The workflow followed by registering parents is controlled by the function woolman_camp_page() in the file [woolman_camp_pages.inc](woolman_camp_pages.inc). Read that code and you will see the following steps (coded in reverse order since we want it to take people to the latest point in their process, not the earliest):

1. Log in/register for a drupal account
2. Complete a "how did you hear about us" activity (if you haven't done so already, AND you are new to camp).
3. Complete/update the family info form. This creates a camp registration activity with status 4 (in progress).
4. Register campers for sessions. This updates the registration activity to status 2 (complete).
5. Await approval, if any registered campers are new. Staff are notified so they can review the registration questions. If any sessions are full, they will also sit at this step while on the waitlist.
6. Complete the emergency medical info, and agree to terms and conditions (creates a camp release forms activity)
7. Pay fees (online or by check) - payments will automatically get the year inserted as their key, and will trigger an update in the participant payment amount.

Date Handling
-------------

Since camp only happens once a year, the test for whether a given activity has been completed or not is based on whether it was done "this year." Since "this year" is not a calendar year, but rather September - August, there is a little function in the woolman_camp.module to simplify dates: woolman_camp_next_year().
