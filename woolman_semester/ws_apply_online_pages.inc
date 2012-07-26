<?php

module_load_include('inc', 'woolman_semester', 'ws_apply_online_utils');

/**
 * Student application page.
 */
function _ws_apply_page_one() {
  drupal_add_css(drupal_get_path('module', 'woolman_semester') . '/css/ws_apply_online.css');

  if (!user_is_logged_in()) {
    return views_embed_view('embed_image', 'attachment_1', 111, '3+4+5')._ws_apply_settings('ws_apply_blurb').'<div class="instructions"><h5>Ready to get started?</h5><p>In order to begin the application process, please '.l('log in','user', array('query' => array('destination' => 'semester/admissions/apply'), 'alias' => TRUE)).'. If you don\'t yet have a Woolman.org account, it only takes a moment to '.l('register', 'user/register', array('query' => array('destination' => 'semester/admissions/apply'), 'alias' => TRUE)). '.</p>'.l('Apply Now', 'user/register', array('query' => array('destination' => 'semester/admissions/apply'), 'alias' => TRUE, 'attributes' => array('class' => 'button-style'))). '</div><div class="instruction-bottom"></div>';
  }
  else {
    module_load_include('inc', 'woolman_semester', 'ws_apply_online_forms');
  }

  // Load the application case, if it exists. Else display "get started" button.
  if (!($case = _ws_apply_get_case())) {
    return views_embed_view('embed_image', 'attachment_1', 111, '3+4+5')._ws_apply_settings('ws_apply_blurb').'<div class="instructions"><h5>Ready to get started? Click here for our online application forms:</h5>'.drupal_get_form(_ws_apply_form_get_started).'</div><div class="instruction-bottom"></div>';
  }

  drupal_set_title(woolman_name('nick') . "'s Application");

  $table = array();
  $status_ids = array(
    0 => 'none',
    1 => 'scheduled',
    2 => 'complete',
    3 => 'waiting',
    4 => 'draft',
    5 => 'waiting',
    6 => 'ready',
  );
  $steps = array(
    35 => array(
      'path' => 'form',
      'name' => 'Application Form',
      'text' => 'Basic information about you and your family.',
    ),
    42 => array(
      'path' => 'school',
      'name' => 'School Information',
      'text' => 'Information about your High School and the teachers who will write your recommendation.',
    ),
    40 => array(
      'path' => 'questions',
      'name' => 'Short-Answer Questions',
      'text' => 'Some questions to help us get to know you.',
    ),
    36 => array(
      'path' => 'essay',
      'name' => 'Essay Question',
      'text' => 'Answer a prompt in traditional essay form.',
    ),
    51 => array(
      'path' => 'financial-aid',
      'name' => 'Financial Aid',
      'text' => 'Indicate if you will be applying for financial aid.',
      'waiting' => 'Click for instructions for submitting financial aid.',
    ),
    41 => array(
      'name' => 'Transcript',
      'text' => 'Please ask your High School registrar to mail or fax us a copy of your transcript.',
      'hover' => 'Our fax number is 530-273-3183',
      'complete' => 'Received %date.',
    ),
    38 => array(
      'name' => 'Parent/Guardian Questionnaire',
      'text' => 'Questions for your parent or guardian to answer. It will be sent to them by email when you fill out the basic application form.',
      'waiting' => 'The questionnaire was emailed to %target %date.',
      'draft' => 'Saved as a draft by %target %date.',
      'complete' => '%target completed the questionnaire %date.',
    ),
    39 => array(
      'name' => 'Teacher Recommendation',
      'text' => 'The school information form will ask you for two teachers, and the forms will be sent to them by email.',
      'waiting' => 'The recommendation form was emailed to %target %date.',
      'draft' => 'Saved as a draft by %target %date.',
      'complete' => '%target submitted the recommendation %date.',
    ),
    37 => array(
      'name' => 'Interview',
      'text' => 'Interview with the admissions staff, by phone or in person. We will schedule it with you once your application forms are complete.',
      'ready' => 'Now that you have completed the application forms, please contact us (<a href="mailto:admissions@woolman.org">admissions@woolman.org</a> or 530-273-3183) to schedule an interview.',
      'scheduled' => 'Your interview is scheduled for %datetime.',
    ),
  );
  $completed = 0;
  foreach ($steps as $type => $step) {
    $acts = woolman_aval($case['acts'], $type, array(array()));
    foreach ($acts as $c => $act) {
      $status = woolman_aval($act, 'status_id', 0);
      if ($type == 37 && $completed >= 4 && !$status) {
        $status = 6;
      }
      // Show financial aid even if it doesn't exist
      if ($type == 51 && !$status && $completed) {
        $status = 6;
      }
      $step += array(
        'complete' => 'Completed %date.',
        'draft' => 'Draft saved %date.'
      );
      $row = array(
        'status' => $status_ids[$status],
        'name' => $step['name'],
        'description' => woolman_aval($step, $status_ids[$status], $step['text']),
        'hover' => woolman_aval($step, 'hover'),
      );
      if ($status && !empty($step['path'])) {
        $row['name'] = l($row['name'], '~semester/admissions/apply/' . $step['path'], array('alias' => TRUE));
      }
      if ($status == 2 && in_array($type, array(35, 42, 40, 36))) {
        $completed++;
      }
      if ($type == 41 && $completed && $status != 2) {
        $row['status'] = 'waiting';
      }
      if ($type != 35 && !$completed && empty($row['hover'])) {
        $row['hover'] = 'Please fill out the basic application form first';
      }
      if (strpos($row['description'], '%date') !== FALSE) {
        $token = strpos($row['description'], '%datetime') !== FALSE ? '%datetime' : '%date';
        $date = _ws_apply_act_date_format($act, $token == '%datetime');
        $row['description'] = str_replace($token, $date, $row['description']);
      }
      if (strpos($row['description'], '%target') !== FALSE &&
      $target = db_result(db_query('SELECT target_contact_id FROM {civicrm_activity_target} WHERE activity_id = %d LIMIT 0, 1', $act['id']))) {
        $row['description'] = str_replace('%target', woolman_name('full', $target), $row['description']);
      }
      $table[] = $row;
    }
  }
  return theme('ws_apply_online_status', $table, 'Application to The Woolman Semester');
}

/**
 * Output themed application status.
 * @param $table
 *   Table data
 * @param $caption
 *   Table caption
 * @return
 *   HTML formatted table.
 */
function theme_ws_apply_online_status($table, $title) {
  $output = '<div id="app-status-container">
              <div id="app-status-content">
                <h3>' . $title . '</h3>'
                . drupal_get_form('_ws_apply_form_semester_pref')
                . '<table id="app-status-table">
                  <tbody>';
  foreach ($table as $type => $row) {
    $title = woolman_aval($row, 'hover');
    $title = $title ? 'title="' . $title . '"' : '';
    $output .= '<tr class="spacer"><td colspan="3"></td></tr>
                <tr class="' . $row['status'] . '">
                 <td class="box"><div></div></td>
                 <td ' . $title . ' class="name">' . $row['name'] . '</td>
                 <td class="description">' . $row['description'] . '</td>
               </tr>';
  }

  $output .=      '</tbody>
                 </table>
               </div>
             <div id="app-status-bottom"></div>
           </div>';
  return $output;
}

/**
 * Application Forms Page Callback
 */
function _ws_apply_forms_request($type) {
  module_load_include('inc', 'woolman_semester', 'ws_apply_online_forms');

  switch($type) {
    case 'form':
      $act_id = 35;
      break;
    case 'essay':
      $act_id = 36;
      break;
    case 'questions':
      $act_id = 40;
      break;
    case 'school':
      $act_id = 42;
      break;
    case 'financial-aid':
      $act_id = 51;
      $type = 'financial_aid';
      break;
    default:
      return drupal_not_found();
  }

  if (!($case = _ws_apply_get_case())) {
    drupal_goto('~semester/admissions/apply');
  }

  $activity = woolman_aval($case['acts'], "$act_id:0", array('status_id' => 0));

  // If they have already completed this step, show submission
  if ($activity ['status_id'] == 2 && $act_id != 51) {
    $type = $type == 'school' ? 'School Form' : $type;
    drupal_set_title('Application '.ucfirst($type).' (Complete)');

    return '<h4>You submitted this form '._ws_apply_act_date_format($activity).':</h4>'.($type=='essay' ? '<h5>' . $activity['subject'] . '</h5>' : '') . $activity['details'] . '<p>' . l('<< Back to Application','~semester/admissions/apply', array('alias' => TRUE)) . '</p>';
  }

  elseif ($type != 'form' && $case['acts'][35][0]['status_id'] != 2) {
    drupal_goto('~semester/admissions/apply');
  }

  // If no previous submission, render form
  else {
    drupal_set_title('Application ' . ucfirst($type));
    return drupal_get_form('_ws_apply_form_type_' . $type);
  }
}

/**
 * Handle requests for parent/teacher reference forms
 * and the thank you page
 */
function _ws_apply_reference_handler() {
  if (empty($_GET['id'])) {
    return drupal_not_found();
  }
  list($student, $self, $aid, $target_id) = explode('-', $_GET['id']);

  // Check url for valid arguments
  if (!(is_numeric($student) && is_numeric($self) && is_numeric($aid) && is_numeric($target_id))) {
    return drupal_not_found();
  }

  // Security check: make sure all args match correct db values
  $query = db_query(
    'SELECT tar.activity_id, tar.target_contact_id, cc.contact_id
      FROM {civicrm_activity_target} tar, {civicrm_case_activity} ca, {civicrm_case_contact} cc
      WHERE tar.id = %d AND tar.activity_id = ca.activity_id AND ca.case_id = cc.case_id', $target_id);
  $db = db_fetch_array($query);
  if (empty($db) || $db['activity_id'] != $aid || $db['target_contact_id'] != $self || $db['contact_id'] != $student) {
    return drupal_not_found();
  }

  if (arg(3) == 'thank-you') {
    $text = _ws_apply_settings('ws_apply_reference_thankyou', TRUE);
    $student = woolman_contact_get($student);
    $self = woolman_contact_get($self);
    return _ws_apply_replace_tokens($text, $student, $self);
  }

  module_load_include('inc', 'woolman_semester', 'ws_apply_online_forms');
  return drupal_get_form('_ws_apply_form_type_reference');
}
