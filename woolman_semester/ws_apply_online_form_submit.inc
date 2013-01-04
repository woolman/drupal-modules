<?php

module_load_include('inc', 'woolman_semester', 'ws_apply_online_utils');

/**
 * Begin Application
 */
function _ws_apply_form_get_started_submit($form, &$form_state) {
  $cid = $params['client_id'] = woolman_user_cid();

  $ok = TRUE;
  foreach (woolman_civicrm_api('case', 'get', $params, 'values') as $case) {
    // Make sure there isn't already an open application case
    if ($case['case_type_id'] == 2 && empty($case['is_deleted'])) {
      $ok = FALSE;
    }
    // Close prospective student cases
    elseif ($case['case_type_id'] == 1 && empty($case['is_deleted']) && $case['status_id'] != 2) {
      $params = array(
        'case_id' => $case['id'],
        'status_id' => 2,
      );
      // Update case status
      woolman_civicrm_api('case', 'create', $params);
      $params += array(
        'activity_type_id' => 16,
        'source_contact_id' => $cid,
        'medium_id' => 7,
        'subject' => 'Online Application Started',
        'details' => '<p>Automatically closed when student began online application.</p>',
      );
      // Add "close case" activity
      woolman_civicrm_api('activity', 'create', $params);
    }
  }
  if ($ok) {
    $params = array(
      'case_type_id' => 2,
      'status_id' => 1,
      'contact_id' => $cid,
      'creator_id' => _ws_apply_settings('ws_apply_admin_id'),
      'subject' => 'Online Application',
      'location' => 'Website',
      'medium_id' => 7,
    );
    $new_case = woolman_civicrm_api('case', 'create', $params);
    // Add semester choice activity
    $sems = array();
    foreach ($form_state['values']['semester_choices'] as $sem => $selected) {
      if ($selected) {
        $sems[$sem] = $form['semester_choices']['#options'][$sem];
      }
    }
    $params = array(
      'case_id' => $new_case['id'],
      'activity_type_id' => 47,
      'source_contact_id' => $cid,
      'status_id' => 2,
      'medium_id' => 7,
      'subject' => implode(' or ', $sems),
      'custom_5' => woolman_multivalue_str(array_keys($sems)),
    );
    $result = woolman_civicrm_api('activity', 'create', $params);

    // Alert staff
    $message = array(
      'subject' => 'Application started for ' . woolman_name('full'),
      'body' => woolman_name('full') . ' just began the application process for ' . implode(' or ', $sems),
    );
    drupal_mail('woolman_website', 0, 'admissions@woolman.org', language_default(), $message, 'info@woolman.org');
  }
}

/**
 * Process basic application form
 */
function _ws_apply_form_basic_submit($form, &$form_state) {
  $vals = $form_state['values'];
  $cid = woolman_user_cid();
  $parent_count = $vals['parent2_last'] ? 2 : 1;
  // Save parents
  for ($i = 1; $i <= $parent_count; $i++) {
    $whose = $vals['parent' . $i . '_same'] ? 'self' : 'parent' . $i;
    $params = array(
      'contact' => array(
        'first_name' => $vals['parent' . $i . '_first'],
        'last_name' => $vals['parent' . $i . '_last'],
        'nick_name' => $vals['parent' . $i . '_nick'],
        'gender_id' => $vals['parent' . $i . '_gender'],
        'birth_date' => woolman_date_array_to_string($vals['parent' . $i . '_dob']),
        'job_title' => $vals['parent' . $i . '_employer']
      ),
      'address' => array(array(
        'location_type_id' => 1,
        'is_primary' => 1,
        'street_address' => $vals[$whose . '_street'],
        'city' => $vals[$whose . '_city'],
        'state_province' => $vals[$whose . '_state'],
        'postal_code' => $vals[$whose . '_zip'],
        'country_id' => $vals[$whose . '_country'],
      )),
      'email' => array(array(
        'email' => $vals['parent' . $i . '_email'],
        'location_type_id' => 1,
        'is_primary' => TRUE,
      )),
    );
    $is_primary = 1;
    foreach (array(1 => 'home', 2 => 'work', 4 => 'cell') as $id => $type) {
      if (!empty($vals['parent' . $i . '_' . $type . '_phone'])) {
        $params['phone'][] = array(
          'phone' => $vals['parent' . $i . '_' . $type . '_phone'],
          'phone_type_id' => 1,
          'location_type_id' => $id,
          'is_primary' => $is_primary,
        );
        $is_primary = 0;
      }
    }
    if ($i == 2 && $vals['parent1_same'] && $vals['parent2_same']) {
      $params['address'] = array('share_with_contact' => $pid[1]);
    }

    $result = woolman_contact_match($params, 'update_all', 'return_cid', 'strict', $cid);
    $pid[$i] = $result['contact_id'];

    // Create relationship
    woolman_civicrm_api('relationship', 'create', array(
      'contact_id_a' => $cid,
      'contact_id_b' => $pid[$i],
      'relationship_type_id' => 1,
      'custom_8' => $vals['parent' . $i . '_relationship'],
      'is_active' => 1
    ));
  }
  // Create parents' relationship
  if ($parent_count == 2 && ($p_r = $vals['parents_relationship'])) {
    $active = 1;
    if ($p_r == 'divorced') {
      $p_r = 2;
      $active = 0;
    }
    woolman_civicrm_api('relationship', 'create', array(
      'contact_id_a' => $pid[1],
      'contact_id_b' => $pid[2],
      'relationship_type_id' => $p_r,
      'is_active' => $active,
    ));
  }
  // Save student
  $vals['self_ethnicity'] = array_flip($vals['self_ethnicity']);
  unset($vals['self_ethnicity'][0]);
  $params = array(
    'contact' => array(
      'contact_id' => $cid,
      'first_name' => $vals['self_first'],
      'last_name' => $vals['self_last'],
      'nick_name' => $vals['self_nick'],
      'gender_id' => $vals['self_gender'],
      'birth_date' => woolman_date_array_to_string($vals['self_dob']),
      'custom_59' => woolman_multivalue_str($vals['self_ethnicity']),
    ),
    'address' => array(array(
      'location_type_id' => 1,
      'is_primary' => 1,
      'street_address' => $vals['self_street'],
      'city' => $vals['self_city'],
      'state_province' => $vals['self_state'],
      'postal_code' => $vals['self_zip'],
      'country_id' => $vals['self_country']
    )),
    'email' => array(array(
      'email' => $vals['self_email'],
      'location_type_id' => 1,
      'is_primary' => TRUE
    )),
  );
  $is_primary = 1;
  foreach (array(4 => 'cell', 1 => 'home') as $id => $type) {
    if (!empty($vals['self_' . $type . '_phone'])) {
      $params['phone'][] = array(
        'phone' => $vals['self_' . $type . '_phone'],
        'phone_type_id' => 1,
        'location_type_id' => $id,
        'is_primary' => $is_primary,
      );
      $is_primary = 0;
    }
  }
  if ($vals['parent1_same']) {
    $params['address'] = array('share_with_contact' => $pid[1]);
  }
  elseif ($vals['parent2_same'] && $parent_count == 2) {
    $params['address'] = array('share_with_contact' => $pid[2]);
  }
  woolman_contact_match($params);

  // Process how student heard about us
  $params = array(
    'activity_type_id' => 53,
    'source_contact_id' => $cid,
    'target_contact_id' => $cid,
    'subject' => $form['self']['self_medium']['#options'][$vals['self_medium']],
    'status_id' => 2,
    'details' => '<p><em>Entered by ' . $vals['self_first'] . ' ' . $vals['self_last'] .
      ' on the Woolman Semester application form:</em></p><p>' . $vals['self_details'] . '</p>',
    'custom_6' => woolman_multivalue_str('woolman semester'),
    'custom_7' => $vals['self_medium'],
  );
  woolman_civicrm_api('activity', 'create', $params);

  // Send reference emails & create activity
  $case = _ws_apply_get_case();
  $student = woolman_contact_get($cid);
  $contact = woolman_contact_get($pid[1]);
  $links[0] = _ws_apply_send_reference('parent', $student, $contact, $case);

  // Add next activities to case
  $params = array(
    'source_contact_id' => $cid,
    'status_id' => 6,
    'medium_id' => 7,
    'case_id' => $case['id'],
    'subject' => 'Not started',
  );
  foreach (array(42, 40, 36, 51) as $type) {
    if (empty($case['acts'][$type])) {
      $params['activity_type_id'] = $type;
      woolman_civicrm_api('activity', 'create', $params);
    }
  }

  // Parse through submission and format all pretty-like
  $details = '<ul>';
  $set = 'self';
  $skip = 0;
  foreach ($vals as $key => $val) {
    if ((strpos($key, 'elf_') || strpos($key, 'arent'))) {
      if (strpos($key, 'arent') && strpos($key, '_first')) {
        if (!$val) {
          break;
        }
        $details .= '</ul><h4>Parent/Guardian:</h4><ul>';
        $set = substr($key, 0, 7);
        $skip = 0;
      }
      if (strpos($key, 'same')) {
        $skip = $val;
        if ($val)
          $details .= '<li><b>Address:</b> Same as Student</li>';
        }
      elseif (!$skip) {
        $details .= '<li><b>' . $form[$set][$key]['#title'] . ':</b> ';
        if (is_array($val))
          $details .= implode('/', $val);
        elseif ($form[$set][$key]['#options'])
          $details .= $form[$set][$key]['#options'][$val];
        else
          $details .= $val;
        $details .= '</li>';
      }
    }
  }
  $details .= '</ul>';
  ws_apply_process_submission(35, 'Application form', $details, $form_state, $case, $links);
}

/**
 * Process school/teacher form
 */
function _ws_apply_form_school_submit($form, &$form_state) {
  $vals = $form_state['values'];
  $student = woolman_contact_get(NULL, array(4));
  $case = _ws_apply_get_case($student['contact_id']);

  //graduation date
  if (($m = $vals['graduation_date']['month']) < 10) {
    $m = '0' . $m;
  }
  $grad_date = $vals['graduation_date']['year'] . $m . '01';
  woolman_civicrm_api('contact', 'create', array(
    'contact_id' => $student['contact_id'],
    'contact_type' => 'Individual',
    'custom_4' => $grad_date,
  ));
  $details = '<p><b>Graduation Date:</b> ' . $m.'/' . $vals['graduation_date']['year'] . '</p>
              <p><b>Student Is:</b> ' . $form['school']['school_type']['#options'][$vals['school_type']] . '</p>';

  if ($vals['school_type'] == 'other') {
    $details .= '<p><b>' . $form['school']['education']['#title'] . ':</b></p><p>'. nl2br($vals['education']) . '</p>';
  }
  else {
    $details .= '<p><b>High School:</b> ';
    if (($school_id = $vals['school_choice']) != 'none') {
      //school was chosen from list
      $school = $form['school']['school_options']['school_choice_wrapper']['school_choice']['#options'][$vals['school_choice']];
      $details .= '(selected from existing choices) ' . $school.', ' . $form['school']['school_options']['school_location']['#options'][$vals['school_location']] . '</p>';
    }
    else {
      $details .= '(manually entered) <ul>';
      $school = $vals['school_name'];
      foreach ($form['school']['details'] as $field=> $info) {
        if (empty($vals[$field])) {
          continue;
        }
        $details .= '<li><b>' . $info['#title'] . ':</b> ' . woolman_aval($info, '#options:' . $vals[$field], $vals[$field]) . '</li>';
      }
      $details .= '</ul></p>';

      // Add school to db
      $params = array(
        'contact' => array(
          'organization_name' => $vals['school_name'],
          'contact_type' => 'Organization',
          'contact_sub_type' => 'High_School',
        ),
      );
      //school address
      list($sch_country, $sch_state) = explode('-', $vals['school_location']);
      $params['address'] = array(array(
        'location_type_id' => 3,
        'is_primary' => 1,
        'street_address' => $vals['school_address'],
        'city' => $vals['school_city'],
        'postal_code' => $vals['school_zip'],
        'state_province_id' => $sch_state,
        'country_id' => $sch_country,
      ));
      $params['phone'] = array(array(
        'location_type_id' => 3,
        'is_primary' => 1,
        'phone' => $vals['school_phone'],
      ));
      if ($vals['school_website'] && $vals['school_website'] != 'http://') {
        $params['website'] = array(array(
          'website_type_id' => 6,
          'is_primary' => 1,
          'website' => $vals['school_website']
        ));
      }
      $result = woolman_contact_match($params);
      $school_id = $result['contact_id'];
    }

    woolman_relationship_add_or_update(array(
      'contact_id_a' => $student['contact_id'],
      'contact_id_b' => $school_id,
      'relationship_type_id' => 10,
      'is_active' => 1,
      'end_date' => $grad_date,
    ));
  }

  //teachers
  for ($i=1; $i<=2; $i++) {
    $params = array(
      'contact' => array(
        'first_name' => $vals['teacher' . $i . '_first'],
        'last_name' => $vals['teacher' . $i . '_last'],
        'gender_id' => $vals['teacher' . $i . '_gender'],
        'job_title' => 'Teacher'),
      'email' => array(array(
        'email' => $vals['teacher' . $i . '_email'],
        'location_type_id' => 2,
        'is_primary' => 1
        ),
      ),
      'phone' => array(array(
        'email' => $vals['teacher' . $i . '_phone'],
        'location_type_id' => $vals['teacher' . $i . '_phone_location'],
        'phone_type_id' => 1,
        'is_primary' => 1
        ),
      ),
    );
    if ($vals['teacher' . $i . '_relationship'] == 'school' && !empty($school_id)) {
      $params['contact']['employer_id'] = $school_id;
    }
    else {
      'Teacher at ' . $vals['teacher' . $i . '_employer_other'];
    }
    $result = woolman_contact_match($params);
    $tid = $result['contact_id'];

    if ($vals['teacher' . $i . '_relationship'] == 'school' && !empty($school_id)) {
      woolman_relationship_add_or_update(array(
        'contact_id_a' => $tid,
        'contact_id_b' => $school_id,
        'relationship_type_id' => 4,
        'is_active' => 1,
      ));
    }

    woolman_relationship_add_or_update(array(
      'contact_id_a' => $student['contact_id'],
      'contact_id_b' => $tid,
      'relationship_type_id' => 14,
      'is_active' => 1,
      'case_id' => $case['id'],
    ));

    //Send reference emails & create activity
    $contact = woolman_contact_get($tid);
    $links[$i] = _ws_apply_send_reference('teacher', $student, $contact, $case);

    $details .= "<p><strong>Teacher $i:</strong><ul>";
    foreach ($form['teacher' . $i] as $field => $info) {
      if (empty($vals[$field]) || $field=='teacher' . $i . '_employer_other') {
        continue;
      }
      if ($field == 'teacher' . $i . '_relationship') {
        $details .= '<li><b>' . $info['#title'] . ':</b> ' . ($vals[$field]=='school' ? $school : $vals['teacher' . $i . '_employer_other']) . '</li>';
      }
      else {
        $details .= '<li><b>' . $info['#title'] . ':</b> '
          . woolman_aval($info, '#options:' . $vals[$field], $vals[$field]) . '</li>';
      }
    }
    $details .= '</ul></p>';
  }
  ws_apply_process_submission(42, $school, $details, $form_state, $case, $links);
}

/**
 * Process essay form
 */
function _ws_apply_form_essay_submit($form, &$form_state) {
  if ($choice = $form_state['values']['choice'])
    $title = $form['choice']['#options'][$choice];
  else
    $title = _ws_apply_settings('ws_apply_essay1');
  $details = $form_state['values']['essay'];

  ws_apply_process_submission(36, $title, $details, $form_state);
}

/**
 * Process short-answer questions form
 */
function _ws_apply_form_questions_submit($form, &$form_state) {
  $details = '';
  $n = 0;
  foreach ($form_state['values'] as $key=> $value) {
    if (strpos($key, 'apply_question')) {
      $details .= '<h5>' . $form[$key]['#title'] . '</h5><p>'.str_replace(array("\n","\r"), '<br />', $value) . '</p>';
      $n++;
    }
  }
  $title = $n.' Short Answer Questions';
  ws_apply_process_submission(40, $title, $details, $form_state);
}

/**
 * Process semester pref form
 */
function _ws_apply_form_semester_pref_submit($form, &$form_state) {
  if (!empty($form_state['ahah_submission'])) {
    return;
  }
  $s = $form_state['storage'];
  $self = woolman_contact_get();
  $his_her = 'their';
  if ($self['gender_id'] == 1) {
    $his_her = 'her';
  }
  if ($self['gender_id'] == 2) {
    $his_her = 'his';
  }
  $sel = array_flip($form_state['values']['semester_choices']);
  unset($sel[0]);
  $choices = $message = array();
  foreach ($sel as $sem) {
    $choices[$sem] = woolman_get_semester($sem);
  }
  $past_choice = implode(' or ', $s['semester']);
  $current_choice = implode(' or ', $choices);
  $status = $s['status_id'];
  if (!$sel) {
    if ($s['status_id'] != 4) {
      $message = array(
        'subject' => woolman_name('full') . ' application withdrawn.',
        'body' => woolman_name('full') . " has withdrawn $his_her application to The Woolman Semester.\n\nApplication was for: $past_choice",
      );
    }
    $status = 4;
  }
  elseif ($s['status_id'] == 4) {
    $status = 1;
    $message = array(
      'subject' => woolman_name('full') . ' application re-instated.',
      'body' => woolman_name('full') . " has re-instated $his_her application to The Woolman Semester.\n\nSemester Preference: $current_choice",
    );
  }
  if ($past_choice != $current_choice || $status != $s['status_id']) {
    if ($sel) {
      $message = $message ? $message : array(
        'subject' => woolman_name('full') . ' semester preference change.',
        'body' => woolman_name('full') . " has changed the semester preference on $his_her Woolman Semester application.\n\nPrevious Choice: $past_choice\n\nCurrent Choice: $current_choice",
      );
      woolman_civicrm_api('activity', 'create', array(
        'id' => $s['activity_id'],
        'subject' => $current_choice,
        'custom_5' => woolman_multivalue_str($sel),
      ));
    }
    elseif ($s['status_id'] != 4) {
      woolman_civicrm_api('activity', 'create', array(
        'case_id' => $s['case_id'],
        'subject' => 'Application Withdrawn',
        'status_id' => 2,
        'source_contact_id' => $self['contact_id'],
        'target_contact_id' => $self['contact_id'],
        'activity_type_id' => 16,
      ));
    }
  }
  if ($status != $s['status_id']) {
    woolman_civicrm_api('case', 'create', array(
      'id' => $s['case_id'],
      'status_id' => $status,
    ));
  }
  if ($s['status_id'] == 4 || $s['status_id'] == 2) {
    woolman_student_relationship($self['contact_id'], $status == 2 ? 'create' : 'delete');
  }
  if ($message) {
    $message['body'] .= "\n\n" . woolman_contact_url($self['contact_id']);
    drupal_mail('woolman_website', 0, 'admissions@woolman.org', language_default(), $message, 'info@woolman.org');
  }
  unset($form_state['storage']);
}

/**
 * Process financial aid
 */
function _ws_apply_form_type_financial_aid_submit($form, &$form_state) {
  $vals = $form_state['values'];
  $act = $form_state['storage']['act'];
  $params = array(
    'custom_61' => $vals['financial'],
    'status_id' => $vals['financial'] ? 5 : 2,
    'subject' => 'Requesting Financial Aid: ' . ($vals['financial'] ? 'Yes' : 'No'),
    'id' => $form_state['storage']['act']['id'],
  );
  // If anything has changed, update act and send message
  if (!(($act['status_id'] == 2 || $act['status_id'] == $params['status_id'])
  && woolman_aval($act, 'custom_61', 9, TRUE) == $vals['financial'])) {
    $message = array(
      'subject' => woolman_name('full') . '\'s financial aid',
      'body' => woolman_name('nick') . ' indicated: ' . ($vals['financial'] ? '"Yes, I will be applying for financial aid."' : '"No, I do not intend to apply for financial aid."'),
    );
    drupal_mail('woolman_website', 0, 'admissions@woolman.org', language_default(), $message, 'info@woolman.org');
    woolman_civicrm_api('activity', 'create', $params);
  }
  unset($form_state['storage']);
  $form_state['redirect'] = '~semester/admissions/apply';
}

/**
 * D6 AHAH callback
 * TODO: Remove in D7
 */
function _ws_apply_school_js() {
  module_load_include('inc', 'woolman_semester', 'ws_apply_online_forms');
  $form = woolman_process_ahah_form();
  switch ($form['#token']) {
    case '_ws_apply_form_type_school':
      $form = $form['school']['school_options']['school_choice_wrapper']['school_choice'];
      break;
    case '_ws_apply_form_semester_pref':
      $form = $form['pref'];
      break;
  }
  unset($form['#prefix'], $form['#suffix']);
  exit(drupal_json(array('status' => TRUE, 'data' => theme('status_messages') . drupal_render($form))));
}

/**
 * Submit for AHAH dropdown
 */
function _ws_apply_form_ahah_submit($form, &$form_state) {
  unset($form_state['submit_handlers']);
  form_execute_handlers('submit', $form, $form_state);
  $form_state['rebuild'] = TRUE;
}

/**
 * Form submission final processing:
 * -update case activity
 * -delete draft
 * -send email to admissions
 * -redirect user
 */
function ws_apply_process_submission($key, $title, $details, &$form_state, $case=NULL, $links=NULL) {
  $vals = $form_state['values'];

  if (!$case) {
    $case = _ws_apply_get_case();
  }

  woolman_civicrm_api('activity', 'create', array(
    'id' => $case['acts'][$key][0]['id'],
    'subject' => $title,
    'status_id' => 2,
    'medium_id' => 7,
    'activity_date_time' => date('YmdHis'),
    'details' => $details,
  ));

  if (!($id = woolman_aval($case, "acts:$key:0:original_id"))) {
    $id = $case['acts'][$key][0]['id'];
  }
  db_query('DELETE FROM {ws_apply_online_drafts} WHERE id = %d', $id);

  $message = array('body' => "You can track the progress of " . woolman_name('full') . "'s application here:\n
  http://woolman.org/staff/admissions/applications");

  switch ($key) {
    case 35:
      $section = 'basic application (and parent questionnaire link)';
      $message['body'] .= "\n\nAn email containing the following personalized link to the parent/guardian questionnaire has been sent to {$vals['parent1_first']} {$vals['parent1_last']}: {$vals['parent1_email']}\n{$links[0]}\n\nIf needed, you may copy this link into an email and re-send it to " . ($vals['parent1_gender']==1 ? 'her':'him');
      break;
    case 36:
      $section = 'essay';
      break;
    case 40:
      $section = 'short answers';
      break;
    case 42:
      $section = 'school info (and teacher recommendation links)';
      $message['body'] .= "\n\nThe following personalized links to the student recommendation form were sent to these teachers:\n\n{$vals['teacher1_first']} {$vals['teacher1_last']}: {$vals['teacher1_email']}\n{$links[1]}\n\n{$vals['teacher2_first']} {$vals['teacher2_last']}: {$vals['teacher2_email']}\n{$links[2]}\n\nIf needed, you may copy a link into an email and re-send it.";
  }
  $message['subject'] = woolman_name('full') . '\'s ' . $section;
  drupal_mail('woolman_website', 0, 'admissions@woolman.org', language_default(), $message, 'info@woolman.org');
  $form_state['redirect'] = '~semester/admissions/apply';
}

/**
 * Process parent/teacher reference forms
 */
function _ws_apply_form_reference_submit($form, &$form_state) {
  $student = $form_state['storage']['student'];
  $self = $form_state['storage']['self'];
  $act = $form_state['storage']['act'];
  $values = $form_state['values'];
  $case = ws_activity_get_case($act['id']);

  switch ($act['activity_type_id']) {
    case 38:  $rec='parent';
              $form_name='parent/guardian questionnaire';
              $title = 'Parent/guardian questionnaire';
              $qs = 12;
    break;
    case 39:  $rec='teacher';
              $form_name='teacher recommendation form';
              $qs = 6;
  }

  $details = '';
  for ($i=1; $i<=$qs; $i++) {
    if (!empty($form['ws_apply_' . $rec . $i])) {
      $details .= '<h4>' . $form['ws_apply_' . $rec . $i]['#title'] . '</h4><p>' . $values['ws_apply_' . $rec . $i] .'</p>';
    }
  }

  if ($rec == 'teacher') {
    $details .= '<h3>Evaluation Rubric</h3><ul>';
    $count = $total = 0;
    for ($i=1; $i<=6; $i++) {
      if (!$form['rubric']['ws_apply_rubric' . $i])
        continue;
      $details .= '<li><em>' . $form['rubric']['ws_apply_rubric' . $i]['#title'] . ':</em> <strong>'.nl2br($values['ws_apply_rubric' . $i]) . ' out of 5</strong></li>';
      ++$count;
      $total += $values['ws_apply_rubric' . $i];
    }
    $avg = $total/$count;
    $title = 'Average Rating: ' . substr("$avg", 0, 3) . ' out of 5';
    $details .= '</ul><strong>' . $title.'</strong>';
  }
  woolman_civicrm_api('activity', 'create',  array(
    'id' => $act['id'],
    'source_contact_id' => $self['contact_id'],
    'target_contact_id' => $student['contact_id'],
    'subject' => $title,
    'status_id' => 2,
    'medium_id' => 7,
    'activity_date_time' => date('YmdHis'),
    'details' => $details,
  ));

  if (!empty($values['mailing_list'])) {
    woolman_civicrm_api('group_contact', 'create', array(
      'contact_id' => $self['contact_id'],
      'group_id' => 2
    ));
  }

  $message['subject'] = "{$self['display_name']}'s $form_name for " . $student['display_name'];

  $message['body'] = "You can track the progress of {$student['first_name']}'s application here:\n
  http://woolman.org/staff/admissions/applications";

  drupal_mail('woolman_website', 0, 'admissions@woolman.org', language_default(), $message, 'info@woolman.org');

  drupal_goto('~semester/admissions/reference/thank-you', 'id=' . $_GET['id']);
}

/**
 * Handles "Save Draft" form button
 */
function _ws_apply_save_draft($form, &$form_state) {
  $aid = $oid = $type = NULL;

  // activity type id
  switch ($form['#id']) {
    case '-ws-apply-form-type-form':
      $type = 35;
      break;
    case '-ws-apply-form-type-school':
      $type = 42;
      break;
    case '-ws-apply-form-type-questions':
      $type = 40;
      break;
    case '-ws-apply-form-type-essay':
      $type = 36;
      break;
    case '-ws-apply-form-type-reference':
      list(, , $oid) = explode('-', $form_state['storage']['id']);
      $aid = $form_state['storage']['act']['id'];
  }

  if ($aid) {
    $link = 'http://semester.woolman.org/admissions/reference?id=' . $form_state['storage']['id'];
    $message = array(
      'subject' => 'Draft Saved',
      'body' =>
"Your reference form for {$form_state['storage']['student']['nick_name']}'s application has been saved as a draft.
To continue working on it, click this link:

$link

-The Woolman Semester Admissions Team
http://semester.woolman.org

Questions? Contact us at:
admissions@woolman.org
530-273-3183",
    );
    drupal_mail('woolman_website', 0, $form_state['storage']['self']['email'], language_default(), $message, 'admissions@woolman.org');
  }
  else {
    $form_state['redirect'] = '~semester/admissions/apply';
    $case = _ws_apply_get_case();
    //use original_id if it exists for compatability with civi activity revisioning system
    $aid = $case['acts'][$type][0]['id'];
    if (!($oid = woolman_aval($case['acts'][$type][0], 'original_id'))) {
      $oid = $aid;
    }
  }

  $record = array(
    'id' => $oid,
    'content' => $form_state['values'],
    'date' => date('Y-m-d H-i-s'),
  );

  // Check for existing record to update
  if (db_result(db_query('SELECT id FROM {ws_apply_online_drafts} WHERE id = ' . $oid))) {
    drupal_write_record('ws_apply_online_drafts', $record, 'id');
  }
  else {
    drupal_write_record('ws_apply_online_drafts', $record);
  }

  //update activity
  woolman_civicrm_api('activity', 'create', array(
    'id' => $aid,
    'activity_date_time' => date('YmdHis'),
    'subject' => 'draft',
    'status_id' => 4,
  ));
  drupal_set_message('Your draft has been saved.');
}
