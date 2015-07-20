<?php

civicrm_initialize();

/**
 * Fetch semester label from option values
 *
 * @param int $id
 * @return string
 */
function woolman_get_semester($id) {
  static $semesters = array();
  if (!$semesters) {
    $semesters = woolman_get_civi_options(80, array('where' => '', 'order' => ''));
  }
  return woolman_aval($semesters, $id, 'unknown');
}

/**
 * Returns $n upcoming semesters
 *
 * @param int $n
 * @return array
 */
function woolman_upcoming_semesters($n = 0) {
  return woolman_get_civi_options(80, array('where' => 'is_active = 1 AND DATE(SUBSTR(value, 1, 10)) > CURDATE()', 'order' => 'value', 'limit' => $n));
}

/**
 * Stores default values for admin settings
 * Returns setting if set, default if not
 * Replaces linebreaks with HTML breaks, if desired
 *
 * @param $key
 * @param bool $linebreaks
 *
 * @return string
 */
function _ws_apply_settings($key, $linebreaks = FALSE) {
  $default = '';
  switch($key) {

    case 'ws_apply_blurb':
      $default =  '<p>Applying to the Woolman Semester is a personalized process; we want to get to know you and learn what unique gifts you would bring to the program, and we want you to find out as much as you can about the Woolman experience to know if it is right for you.</p>'.
      '<p>The Application process involves the following steps:</p>'.
      '<ol><li>Basic application (name, address, etc.)</li>'.
          '<li>Essay and short-answer questions</li>'.
          '<li>Two letters of recommendation</li>'.
          '<li>Sending us your High School transcript</li>'.
          '<li>A phone interview with one of our admissions staff</li>'.
          '<li>Coming to Woolman as a student!</li></ol>';
      break;

    case 'ws_apply_financial_aid_instructions':
      $default =  '<h4>How do I apply?</h4><ol><li>Go to <a href="https://www12.student-1.com/SSSNet/" target="_blank">http://sss.nais.org</a></li><li>Click on the prompt to begin your Parent&#39;s Financial Statement (PFS). <strong>Our school code is 902016</strong>.</li><li>You will be given a password which will allow you to return to your PFS at a later date before submitting it.</li><li>Call SSS at 800.344.8328 with any questions.</li></ol><p><em>Online applications are less expensive and SSS processes the application and delivers the materials to The Woolman Semester School more quickly. </em><em>If the PFS is completed online, the fee is $35. If the PFS is submitted&nbsp; on paper, the fee is $42. These are flat fees, regardless of the number of schools to which you are applying for financial aid. If you would like us to mail you a paper version of these forms, please let us know.</em></p>';
      break;

    case 'ws_apply_essay1':
      $default =  'Describe a social justice or environmental issue that is important to you. How have you contributed or how would you like to contribute to a solution?';
      break;

    case 'ws_apply_question1':
      $default =  'Based on what you know about our program, why are you drawn to attending Woolman? What do you hope to gain from your experience here?';
      break;

    case 'ws_apply_basic_form_instructions':
      $default = 'Please tell us some basic information about yourself and your family.';
      break;

    case 'ws_apply_essay_instructions':
      $default = 'Please choose one of the following questions to answer. Your answer should be in traditional essay format, with an introduction, body, and conclusion.';
      break;

    case 'ws_apply_questions_instructions':
      $default = 'Please answer all of the following questions. Your answers do not need to be in essay form, but please write in complete sentences.';
      break;

    case 'ws_apply_school_instructions':
      $default = 'Please tell us about your school.';
      break;

    case 'ws_apply_school2_instructions':
      $default = 'Please name the two teachers who will be writing you letters of recommmendation:';
      break;

    case 'ws_apply_teacher_instructions':
    case 'ws_apply_parent_instructions':
      $default = "Welcome {contact.first_name},\nThanks for taking the time to add your valuable perspective to this application process. Please answer the following questions to the best of your ability. If you have any questions, please don't hesitate to contact us.";
      break;

    case 'ws_apply_teacher_subject':
    case 'ws_apply_parent_subject':
      $default = 'Reference request for {student.first_name} {student.last_name}\'s application to the Woolman Semester';
      break;

    case 'ws_apply_teacher_email':
      $default = "Dear {contact.first_name} {contact.last_name},\n\nAs part of {his/her} application for the Woolman Semester, {student.first_name} {student.last_name} listed you as a reference. Please take a moment to fill our our student recommendation form by clicking on the following link:\n\n{link}\n\nYour valuable perspective is greatly appreciated.\n\nThank you!\n\nThe Woolman Semester Admissions team\nhttp://semester.woolman.org";
      break;

    case 'ws_apply_parent_email':
      $default = "Dear {contact.first_name} {contact.last_name},\n\nAs you probably know, {student.first_name} is applying to be a student at the Woolman Semester. Please take a moment to fill our our parent/guardian questionnaire by clicking on the following link:\n\n{link}\n\nYour valuable perspective is greatly appreciated.\n\nThank you!\n\nThe Woolman Semester Admissions team\nhttp://semester.woolman.org";
      break;

    case 'ws_apply_parent1':
      $default = 'Please describe {student.first_name}\'s interactions with family and friends';
      break;

    case 'ws_apply_teacher1':
      $default = 'How long and in what capacity have you known {student.first_name}?';
      break;

    case 'ws_apply_rubric1':
      $default = 'How well does {student.first_name} take initiative?';
      break;

    case 'ws_apply_reference_thankyou':
      $default = 'Thank you for your valuable input, and for supporting {student.first_name}\'s education. We encourage you to find out more about our program by browsing our website.';
      break;

    case 'ws_apply_admin_id':
      $query = db_query("SELECT contact_id FROM {civicrm_email} WHERE email='admissions@woolman.org' LIMIT 0, 1");
      $target = db_fetch_array($query);
      $default = woolman_aval($target, 'contact_id');
      break;
  }

  if (!$default && substr($key, 21) == 'ws_apply_rubric_scale') {
    $default = 'Rarely,Frequently,Always';
  }

  $value = variable_get($key, $default);

  return $linebreaks ? nl2br($value) : $value;
}

/**
 * Pulls up the application case (or any type of case)
 * Will default to current user if no cid provided
 *
 * @param int $cid
 * @param int $type
 * @param bool $get_acts
 *
 * @return array
 */
function _ws_apply_get_case($cid = NULL, $type = 2, $get_acts = TRUE) {
  if (!$cid) {
    $cid = woolman_user_cid();
  }
  foreach (woolman_civicrm_api('case', 'get', array('client_id' => $cid), 'values') as $case) {
    if ($case['case_type_id'] == $type && !$case['is_deleted']) {
      if ($get_acts) {
        //Load activity info
        $case['acts'] = array();
        $db = db_query('SELECT * FROM {civicrm_activity} WHERE is_current_revision = 1 AND is_deleted = 0 AND is_test = 0 AND id IN (SELECT activity_id FROM {civicrm_case_activity} WHERE case_id = %d)', $case['id']);
        while ($act = db_fetch_object($db)) {
          $case['acts'][$act->activity_type_id][] = (array) $act;
        }
      }
      return $case;
    }
  }
}

/**
 * @return array
 */
function ws_apply_case_status_options() {
  static $stats;
  if (!$stats) {
    $stats = woolman_get_civi_options('case_status');
    $stats[2] = 'Enrolled';
    $stats[3] = 'Needs Attn';
  }
  return $stats;
}

/**
 * @return array
 */
function ws_application_steps() {
  return array(
    'Basic Form' => 35,
    'School Form' => 42,
    'Short Answer' => 40,
    'Essay' => 36,
    'Parent Q/A' => 38,
    'Teacher 1 Rec' => 39,
    'Teacher 2 Rec' => 39,
    'Transcript' => 41,
    'Interview' => 37,
    'Acceptance' => 50,
    'Financial Aid' => 51,
    'Contract' => 52,
  );
}

/**
 * Gets case id of an activity
 *
 * @param int $aid
 * @return bool
 */
function ws_activity_get_case($aid) {
  return db_result(db_query('SELECT case_id FROM {civicrm_case_activity} WHERE activity_id = %d', $aid));
}

/**
 * Gets current revision of an activity
 *
 * @param int $aid
 * @return array
 */
function ws_admissions_fetch_current_act($aid) {
  if ($aid) {
    $resource = db_query('SELECT * FROM {civicrm_activity}
    WHERE is_current_revision = 1 AND (id = %d OR original_id = %d OR original_id = (SELECT original_id FROM {civicrm_activity} WHERE id = %d))', $aid, $aid, $aid);
    if ($act = db_fetch_array($resource)) {
      return $act;
    }
  }
  return array();
}

/**
 * Create/remove student relationship to Woolman
 *
 * @param int $cid
 * @param string $op
 */
function woolman_student_relationship($cid, $op) {
  if ($op == 'create') {
    $pref = woolman_semester_pref($cid);
    if ($semester = woolman_aval($pref, 'semester')) {
      $pref = array_keys($semester);
      $semester = $pref[0];
    }
    else {
      return;
    }
  }
  switch ($op) {
    case 'create':
      $params = array(
        'relationship_type_id' => 10,
        'contact_id_a' => $cid,
        'contact_id_b' => WOOLMAN,
      );
      list($params['start_date'], $params['end_date']) = explode('--', $semester);
      woolman_relationship_add_or_update($params, TRUE);
      break;

    case 'delete':
      /*
      // TODO: This isn't working because of a CiviCRM api bug (relationship update crashes)
      // Uncomment this code when fixed and remove direct query
      foreach (woolman_find_relations($cid, '10', FALSE) as $id => $rel) {
        if ($rel['contact_id'] == WOOLMAN) {
          woolman_civicrm_api('relationship', 'delete', array('id' => $id));
        }
      }*/
      db_query('DELETE FROM {civicrm_relationship} WHERE contact_id_a = %d AND contact_id_b = %d AND relationship_type_id = 10', $cid, WOOLMAN);
      break;
  }
}

/**
 * Gets semester preference for a student
 *
 * @param int $cid
 * @return array
 */
function woolman_semester_pref($cid) {
  $db = db_query("SELECT sc.semesters_5, act.id activity_id, cas.id case_id, cas.status_id
    FROM {civicrm_value_semester_choices_3} sc, {civicrm_case_contact} cc, {civicrm_activity} act, {civicrm_case_activity} ca, {civicrm_case} cas
    WHERE cc.contact_id = %d AND act.id = ca.activity_id AND ca.case_id = cc.case_id AND cas.id = cc.case_id AND act.is_current_revision = 1 AND act.is_deleted = 0 AND act.activity_type_id = 47 AND cas.is_deleted <> 1 AND act.id = sc.entity_id
    ORDER BY cas.case_type_id DESC
    LIMIT 1", $cid);
  if ($pref = db_fetch_array($db)) {
    $sems = woolman_multivalue_arr($pref['semesters_5']);
    $pref['semester'] = array();
    foreach ($sems as $s) {
      $pref['semester'][$s] = woolman_get_semester($s);
    }
    ksort($pref['semester']);
    return $pref;
  }
  return array('semester' => array(), 'status_id' => 0, 'case_id' => 0, 'activity_id' => 0);
}

/**
 * Make date look pretty
 *
 * @param array|string $act
 *   activity array fetched from civicrm activity api OR timestamp
 * @param bool $time
 *   whether to return the time as well as the date
 *
 * @return string
 *   Month, Date (time, if desired) (year, if not this year)
 */
function _ws_apply_act_date_format($act, $time = FALSE) {
  if (is_array($act)) {
    $date = strtotime($act['activity_date_time']);
  }
  else {
    $date = strtotime($act);
  }
  $date_display = date('F jS', $date);
  $test = date('FjY', $date);
  if ($test == date('FjY')) {
    $date_display = 'today';
  }
  elseif ($test == date('FjY', strtotime('-1 day'))) {
    $date_display = 'yesterday';
  }
  if ($time) {
    $date_display .= ' at ' . date('g:i a', $date);
  }
  elseif (date('Y', $date) != date('Y')) {
    $date_display .= ', ' . date('Y', $date);
  }
  return $date_display;
}

/**
 * Schedule activity and send request for reference to specified contact
 *
 * @param string $key
 *   'parent' or 'teacher'
 * @param array $student
 *   full array of information about the student
 * @param array $contact
 *   full array of information about the person writing the rec
 * @param array $case
 *   as returned by _ws_apply_get_case
 *
 * @return string
 */
function _ws_apply_send_reference($key, $student, $contact, $case) {
  //Create new case activity
  $params = array(
    'case_id' => $case['id'],
    'subject' => 'Request sent',
    'activity_type_id' => $key == 'parent' ? 38 : 39,
    'source_contact_id' => $student['contact_id'],
    'target_contact_id' => $contact['contact_id'],
    'status_id' => 5,
    'medium_id' => 7,
  );
  $result = woolman_civicrm_api('activity', 'create', $params);
  $act = $params['id'] = $result['id'];

  $query = db_query("SELECT id FROM {civicrm_activity_contact} WHERE activity_id = $act AND record_type_id = 3 AND contact_id = {$contact['contact_id']}");
  $target = db_fetch_array($query);
  $link = 'http://semester.woolman.org/admissions/reference?id=' . $student['contact_id'] . '-' . $contact['contact_id'] . '-' . $act . '-' . $target['id'];

  // Update activity with link
  $params['details'] = '<p>This personalized link was sent to ' . woolman_contact_link($contact['first_name'] . ' ' . $contact['last_name'], $contact['contact_id']) . ' (<a href="mailto:' . $contact['email'] . '">' . $contact['email'] . '</a>) on ' . date('M j Y') . ':</p><p>' . $link . '</p>';
  woolman_civicrm_api('activity', 'create', $params);

  $subject = _ws_apply_settings('ws_apply_' . $key . '_subject');
  $body = _ws_apply_settings('ws_apply_' . $key . '_email');

  $message = array(
    'subject' => _ws_apply_replace_tokens($subject, $student, $contact, $link),
    'body' => _ws_apply_replace_tokens($body, $student, $contact, $link),
  );

  drupal_mail('woolman_website', 0, $contact['email'], language_default(), $message, 'admissions@woolman.org');

  return $link;
}

/**
 * Token replacement for emails and forms
 *
 * @param string $str
 * @param array $student
 * @param array $contact
 * @param string $link
 * @return string
 */
function _ws_apply_replace_tokens($str, $student, $contact, $link = '') {
  $hisher = $student['gender_id'] == 1 ? 'her' : 'his';
  $heshe = $student['gender_id'] == 1 ? 'she' : 'he';
  $himher = $student['gender_id'] == 1 ? 'her' : 'him';

  $patterns = array('{student.first_name}', '{student.last_name}',
                    '{contact.first_name}', '{contact.last_name}',
                    '{he/she}', '{she/he}', '{his/her}', '{her/his}', '{him/her}', '{her/him}',
                    '{link}',
                  );
  $replacements = array($student['first_name'], $student['last_name'],
                        $contact['first_name'], $contact['last_name'],
                        $heshe, $heshe, $hisher, $hisher, $himher, $himher,
                        $link,
                      );
  return str_ireplace($patterns, $replacements, $str);
}
