Woolman Website Module
======================

_A collection of hooks, custom code, and utilitiy functions_

* [node_form.js](node_form.js):
  * UI for creating subdomain aliases (remove on upgrade when we ditch subdomains)
  * UI improvements for the imagefield module
* [woolman_blog](woolman_blog.inc):
  * Workflows for submitting/publishing blog articles (could potentially use Rules module instead)
* [woolman_directory](woolman_directory.inc):
  * A custom-coded "view" of civicrm contacts taylored for the needs of staff (are staff still using this? if so could we use Views module instead of custom code?)
* [woolman_dupe_query](woolman_dupe_query.inc):
  * Uses [hook_civicrm_dupeQuery](https://wiki.civicrm.org/confluence/display/CRMDOC/hook_civicrm_dupeQuery) to make Civi's dupe rules way smarter (this worked quite well when I wrote it but that was years ago and there are no tests)
*  [woolman_phonathon](woolman_phonathon.inc):
  *  A little custom code to augment the use of civi-webforms for fundraising calls (probably upgrading to the new version of webform_civicrm will obsolete this)
*  [woolman_sending_schools](woolman_sending_schools.inc):
  *  Displays a page about High Schools our students have come from (requires a complex query, not sure if Views could handle it)
*  [woolman_website.module](woolman_website.module):
  * Hook implementations
  * Utility functions
*  [woolman_website_utils.inc](woolman_website_utils.inc):
  * More utility functions
