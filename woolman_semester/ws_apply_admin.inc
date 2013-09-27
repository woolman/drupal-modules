<?php

module_load_include('inc', 'woolman_semester', 'ws_apply_online_utils');

/**
 * Manage semester dates
 */
function ws_apply_staff_dates(&$form_state) {
  jquery_ui_add(array('ui.datepicker'));
  drupal_add_js(drupal_get_path('module', 'woolman_semester') . '/js/ws_apply_admin.js');

  $form = $sems = array();
  $form['#prefix'] = '<p>Enter dates of upcoming semesters. These are displayed to prospective students on the website and application form, and are used for automatically adding students when they are accepted.</p>';
  $form['table'] = array('#value' => '<table id="semester-dates"><thead><tr><th>Semester</th><th>Start Date</th><th>End Date</th></tr></thead><tbody>');
  $options = woolman_civicrm_api('option_value', 'get', array('option_group_id' => 80, 'options' => array('limit' => 9999)), 'values');
  foreach($options as $o) {
    list($o['start'], $o['end']) = explode('--', $o['value']);
    $sems[$o['name']] = $o;
  }
  $form_state['#semesters'] = $sems;
  $this_year = date('Y');
  $row = array(
    '#type' => 'textfield',
    '#size' => 15,
    '#attributes' => array('class' => 'datepicker'),
  );
  for ($y = $this_year; $y <= $this_year + 6; ++$y) {
    for ($s = 1; $s <= 2; ++$s) {
      if ($y == $this_year && $s == 1 && date('n') > 6) {
        continue;
      }
      $label = $y . ($s == 1 ? ' Spring' : ' Fall');
      $name = $y . ($s == 1 ? '_Spring' : '_Fall');
      $form[$name . '_start'] = $row + array(
        '#prefix' => '<tr class="'. ($s == 1 ? 'odd' : 'even') .'"><td>' . $label . '</td><td>',
        '#suffix' => '</td>',
      );
      $form[$name . '_end'] = $row + array(
        '#prefix' => '<td>',
        '#suffix' => '</td></tr>',
      );
      if ($sem = woolman_aval($sems, $name)) {
        $form[$name . '_start']['#default_value'] = date('m/d/Y', strtotime($sem['start']));
        $form[$name . '_end']['#default_value'] = date('m/d/Y', strtotime($sem['end']));
      }
      else {
        $m = $s == 1 ? 0 : 7;
        $form[$name . '_start']['#attributes']['data-default'] = date('Y-m', strtotime($y . '-0' . $m . '-01'));
        $form[$name . '_end']['#attributes']['data-default'] = date('Y-m', strtotime($y . '-' . ($m + 4) . '-01'));
      }
    }
  }
  $form['endtable'] = array('#value' => '</tbody></table>');
  $form['submit'] = array('#type' => 'submit', '#value' => 'Save');
  return $form;
}

/**
 * Submission handler to update semester dates
 */
function ws_apply_staff_dates_submit($form, &$form_state) {
  $vals = $form_state['values'];
  $sems = $form_state['#semesters'];
  $weight = $created = 0;
  // Find max weight of existing options
  foreach ($sems as $s) {
    $weight = $weight < $s['weight'] ? $s['weight'] : $weight;
  }
  $this_year = date('Y');
  for ($y = $this_year; $y <= $this_year + 6; ++$y) {
    for ($s = 1; $s <= 2; ++$s) {
      if ($y == $this_year && $s == 1 && date('n') > 6) {
        continue;
      }
      $name = $y . ($s == 1 ? '_Spring' : '_Fall');
      if (($start = woolman_aval($vals, $name . '_start')) && ($end = woolman_aval($vals, $name . '_end'))) {
        $start = date('Y-m-d', strtotime($start));
        $end = date('Y-m-d', strtotime($end));

        $params = array(
          'value' => $start . '--' . $end,
          'option_group_id' => 80,
          'label' => $y . ($s == 1 ? ' Spring' : ' Fall'),
          'name' => $name,
          'is_active' => 1,
        );
        // Update existing semester dates
        if ($sem = woolman_aval($sems, $name)) {
          if ($start != $sem['start'] || $end != $sem['end']) {
            woolman_civicrm_api('option_value', 'create', $params + array('id' => $sem['id']));
            db_query("UPDATE {civicrm_relationship} SET start_date = '%s', end_date = '%s'
              WHERE (start_date = '%s' OR end_date = '%s')
              AND relationship_type_id = 10 AND contact_id_b = " . WOOLMAN,
              $start, $end, $sem['start'], $sem['end']
            );
            db_query("UPDATE {civicrm_value_semester_choices_3} SET semesters_5 = REPLACE(semesters_5, '%s', '%s')",
              "{$sem['start']}--{$sem['end']}", "{$start}--{$end}");
            drupal_set_message($params['label'] . ' Dates Updated');
          }
        }
        else {
          woolman_civicrm_api('option_value', 'create', $params + array('weight' => ++$weight));
          ++$created;
        }
      }
    }
  }
  if ($created) {
    drupal_set_message($created . ' New Semester' . ($created > 1 ? 's' : '') . ' Saved');
  }
}

/**
 * Admin Form: allows staff to customize the application process
 */
function _ws_apply_form_admin() {
  // Warn of danger!
  drupal_add_js("jQuery('#edit-reset').click(function() {return confirm('This will erase ALL application questions and delete ALL introductory text from the application forms. Are you sure?');});", 'inline', 'footer');

  // Our staff
  $resource = db_query("SELECT display_name, id FROM {civicrm_contact} WHERE employer_id = '" . WOOLMAN . "' ORDER BY sort_name");
  while ($row = db_fetch_array($resource)) {
    $employees[$row['id']] = $row['display_name'];
  }
  $form = array(
    'admin' => array(
      '#type' => 'fieldset',
      '#title' => '<b>Administration</b>'
    ),
  );
  $form['admin']['ws_apply_admin_id'] = array(
    '#type' => 'select',
    '#title' => 'Admissions Person',
    '#description' => 'Who will new application cases be assigned to by default?<br />Can\'t find the right staff member on this list? Make sure they have Woolman set as their current employer in CiviCRM.',
    '#options' => $employees,
    '#default_value' => _ws_apply_settings('ws_apply_admin_id'),
  );
  $form['admin']['ws_apply_blurb'] = array(
    '#type' => 'textarea',
    '#title' => 'Introductory Text',
    '#default_value' => _ws_apply_settings('ws_apply_blurb'),
    '#description' => 'Shown at the top of the Apply Online page for people who have not yet started their application.',
    '#required' => TRUE,
  );
  $form['admin']['format'] = filter_form();
  $form['basics'] = array(
    '#type' => 'fieldset',
    '#title' => '<b>Basics</b>',
  );
  $form['basics']['ws_apply_basic_form_instructions'] = array(
    '#type' => 'textarea',
    '#title' => 'Basic Application Instructions',
    '#rows' => 3,
    '#description' => 'Instructions displayed at the top of the basic application form.',
    '#default_value' => _ws_apply_settings('ws_apply_basic_form_instructions'),
  );
  $form['basics']['ws_apply_financial_aid_instructions'] = array(
    '#type' => 'textarea',
    '#title' => 'Financial Aid Instructions',
    '#rows' => 3,
    '#description' => 'Instructions displayed on the finanancial aid form.',
    '#default_value' => _ws_apply_settings('ws_apply_financial_aid_instructions'),
  );
  $form['school'] = array(
    '#type' => 'fieldset',
    '#title' => '<b>School/Teacher Form</b>',
  );
  $form['school']['ws_apply_school_instructions'] = array(
    '#type' => 'textfield',
    '#title' => 'School Section Instructions',
    '#size' => 130,
    '#maxlength' => 1024,
    '#description' => 'Instructions to students when filling in information about their school.',
    '#default_value' => _ws_apply_settings('ws_apply_school_instructions'),
  );
  $form['school']['ws_apply_school2_instructions'] = array(
    '#type' => 'textfield',
    '#title' => 'Teacher Section Instructions',
    '#size' => 130,
    '#maxlength' => 1024,
    '#description' => 'Instructions to students when entering their teacher references.',
    '#default_value' => _ws_apply_settings('ws_apply_school2_instructions'),
  );
  $form['essays'] = array(
    '#type' => 'fieldset',
    '#title' => '<b>Essay Questions</b>',
  );
  $form['essays']['ws_apply_essay_instructions'] = array(
    '#type' => 'textarea',
    '#title' => 'Essay Instructions',
    '#rows' => 3,
    '#description' => 'Instructions to students when choosing and answering the essay question.',
    '#default_value' => _ws_apply_settings('ws_apply_essay_instructions'),
    '#suffix' => '<div><b>The applicant will be allowed to choose one of the following options for their essay question (enter up to four):</b></div>'
  );
  for ($i=1; $i<=4; $i++) {
    $form['essays']['ws_apply_essay' . $i] = array(
      '#type' => 'textfield',
      '#title' => 'Essay ' . $i,
      '#size' => 130,
      '#maxlength' => 1024,
      '#default_value' => _ws_apply_settings('ws_apply_essay' . $i),
      '#required' => ($i==1),
    );
  }
  $form['questions'] = array(
    '#type' => 'fieldset',
    '#title' => '<b>Short Answer Questions</b>',
  );
  $form['questions']['ws_apply_questions_instructions'] = array(
    '#type' => 'textarea',
    '#title' => 'Short Answer Instructions',
    '#rows' => 3,
    '#description' => 'Instructions to students when answering the short-answer questions.',
    '#default_value' => _ws_apply_settings('ws_apply_questions_instructions'),
    '#suffix' => '<div><b>The applicant will be required to answer all of the following in short answer form (enter up to four):</b></div>'
  );
  for ($i=1; $i<=4; $i++) {
    $form['questions']['ws_apply_question' . $i] = array(
      '#type' => 'textfield',
      '#title' => 'Question ' . $i,
      '#size' => 130,
      '#maxlength' => 1024,
      '#default_value' => _ws_apply_settings('ws_apply_question' . $i),
      '#required' => ($i==1),
    );
  }
  $form['emails'] = array(
    '#type' => 'fieldset',
    '#title' => '<b>Emails</b>',
    '#description' => '<p><i>These emails will be automatically sent from admissions@woolman.org to one parent and two teachers:</i></p>Email replacement tokens: <ul><li><b>{student.first_name}, {student.last_name}</b> -- replaced with first and last name of applicant, respectively.</li><li><b>{contact.first_name}, {contact.last_name}</b> -- person recieving the email</li><li><b>{he/she}, {his/her}, {him/her}</b> -- these will provide the appropriate pronoun depending on gender of the student.</li><li><b>{link}</b> -- link to the online form</li></ul>'
  );

  for ($rec='teacher'; $rec; $rec=($rec=='parent'? NULL : 'parent')) {
    $form['emails']['ws_apply_' . $rec.'_subject'] = array(
      '#type' => 'textfield',
      '#title' => ucfirst($rec).' message subject',
      '#description' => 'Subject for email sent to ' . $rec.'s.',
      '#size' => 130,
      '#maxlength' => 1024,
      '#default_value' => _ws_apply_settings('ws_apply_' . $rec.'_subject'),
    );
    $form['emails']['ws_apply_' . $rec.'_email'] = array(
      '#type' => 'textarea',
      '#title' => ucfirst($rec).($rec=='parent' ? ' Questionnaire' : ' Recommendation').' Email',
      '#description' => 'This email will be sent to the ' . $rec.', with a link to the form. You must include the {link} token somewhere in the message.',
      '#rows' => 13,
      '#default_value' => _ws_apply_settings('ws_apply_' . $rec.'_email'),
    );

    $form[$rec] = array(
      '#type' => 'fieldset',
      '#title' => '<b>'.($rec=='parent'?'Parent Questionnaire':'Teacher Recommendation').' Form</b>',
      '#description' => 'These tokens can help personalize the instructions and questions: <ul><li><b>{student.first_name}, {student.last_name}</b> -- replaced with first and last name of applicant, respectively.</li><li><b>{contact.first_name}, {contact.last_name}</b> -- names of ' . $rec.' filling out the form</li><li><b>{he/she}, {his/her}, {him/her}</b> -- these will provide the appropriate pronoun depending on gender of the student.</li></ul>',
    );
    $qs = ($rec=='parent'? 12 : 6);
    $form[$rec]['ws_apply_' . $rec.'_instructions'] = array(
      '#type' => 'textarea',
      '#title' => ($rec=='parent'?'Parent Questionnaire':'Teacher Recommendation').' Instructions',
      '#description' => 'Instructions to ' . $rec.'s when filling out the form.',
      '#default_value' => _ws_apply_settings('ws_apply_' . $rec.'_instructions'),
      '#suffix' => '<div><b>Enter up to ' . $qs.' questions for the ' . $rec.' to answer:</b></div>'
    );
    for ($i=1; $i<=$qs; $i++) {
      $form[$rec]['ws_apply_' . $rec . $i] = array(
      '#type' => 'textfield',
      '#title' => 'Question ' . $i,
      '#size' => 130,
      '#maxlength' => 1024,
      '#default_value' => _ws_apply_settings('ws_apply_' . $rec . $i),
      '#required' => ($i==1),
      );
    }
  }
  $form['teacher']['rubric'] = array(
    '#type' => 'fieldset',
    '#title' => '<b>Rubric Questions</b>',
    '#description' => '<p>Teachers will be asked to answer the following questions on a scale of 1 to 5. The options at the right let you qualify those numbers for each question:</p>'
  );
  for ($i=1; $i<=6; $i++) {
    $form['teacher']['rubric']['ws_apply_rubric' . $i]  = array(
      '#type' => 'textfield',
      '#title' => 'Rubric Question ' . $i,
      '#default_value' => _ws_apply_settings('ws_apply_rubric' . $i),
      '#size' => 80,
      '#maxlength' => 1024,
      '#required' => ($i==1),
      '#prefix' => '<div class="subset">',
    );
    $form['teacher']['rubric']['ws_apply_rubric_scale' . $i] = array(
      '#type' => 'select',
      '#title' => "Scale",
      '#options' => array(
        'Rarely,Frequently,Always' => '1: Rarely, 3: Frequently, 5: Always',
        'Below Average,Above Average,Top Notch' => '1: Below Average, 3: Above Average, 5: Top Notch',
        'Without Enthusiasm,Fairly Strongly,Enthusiastically' => '1: Without Enthusiasm, 3: Fairly Strongly, 5: Enthusiastically',
      ),
      '#default_value' => variable_get('ws_apply_rubric_scale' . $i, 'Rarely,Frequently,Always'),
      '#suffix' => '</div>'
    );
  }
  $form['thankyou'] = array(
    '#type' => 'fieldset',
    '#title' => 'Parent/Teacher Thank You Page',
  );
  $form['thankyou']['ws_apply_reference_thankyou'] = array(
    '#type' => 'textarea',
    '#title' => 'Message after completing the student reference form',
    '#rows' => 16,
    '#description' => 'A nice, friendly page thanking parents and teachers for filling out the reference form. (will also include a pretty picture, and blog, projects &amp; iwitness sidebar as well). All the aforementioned tokens will work here.',
    '#default_value' => _ws_apply_settings('ws_apply_reference_thankyou'),
  );
  return system_settings_form($form);
}

/**
 * Validation for Admin Form
 */
function _ws_apply_form_admin_validate($form, &$form_state) {
  //these fieldsets allow tokens
  $token = array('emails', 'teacher', 'parent', 'thankyou');
  //these don't
  $non = array('admin', 'basics', 'school', 'essays', 'questions');
  //allowed tokens
  $tokens = array('{student.first_name}', '{student.last_name}',
                  '{contact.first_name}', '{contact.last_name}',
                  '{he/she}', '{she/he}', '{his/her}', '{her/his}', '{him/her}', '{her/him}',
                  );

  $tokenfields = array_keys($form['teacher']['rubric']);
  foreach ($token as $i) {
    $tokenfields = array_merge($tokenfields, array_keys($form[$i]));
  }
  $nontokenfields = array();
  foreach ($non as $i) {
    $nontokenfields = array_merge($nontokenfields, array_keys($form[$i]));
  }

  foreach ($nontokenfields as $id) {
    if (strpos($id, 'ws_apply') === FALSE) {
      continue;
    }
    if (strpos($form_state['values'][$id], '{') !== FALSE || strpos($form_state['values'][$id], '}') !== FALSE) {
      form_set_error($id, 'This field doesn\'t allow tokens.');
    }
  }

  foreach ($tokenfields as $id) {
    if (strpos($id, 'ws_apply') === FALSE) continue;

    $field = str_ireplace($tokens, '', $form_state['values'][$id]);
    if (strpos($id, 'email') || strpos($id, 'subject'))
      $field = str_ireplace('{link}', '', $field);

    if (strpos($field, '{') !== FALSE || strpos($field, '}') !== FALSE) {
      form_set_error($id, 'Invalid token. Please check your spelling and refer to the list of allowed tokens.');
    }
  }
}
