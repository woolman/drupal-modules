<?php

module_load_include('inc', 'woolman_semester', 'ws_apply_online_utils');

/**
 * Displays information about applications to staff
 */
function _ws_apply_report($arg) {
  drupal_add_css(drupal_get_path('module', 'woolman_semester') . '/css/ws_apply_staff.css', 'module', 'all', FALSE);
  drupal_add_css('sites/all/libraries/jquery.ui/themes/default/ui.all.css', 'module', 'all', FALSE);
  drupal_add_js('sites/all/libraries/tablesorter/jquery.tablesorter.min.js');
  drupal_add_js('sites/all/libraries/ckeditor/ckeditor_basic.js', 'module', 'header', TRUE, FALSE);
  jquery_ui_add(array('ui.dialog', 'ui.datepicker'));
  drupal_add_js(drupal_get_path('module', 'woolman_semester') . '/js/ws_apply_staff.js');
  drupal_add_js('misc/autocomplete.js');
  drupal_add_js('misc/textarea.js');
  drupal_add_js('misc/collapse.js');

  $semester = $arg = $arg ? $arg : 'upcoming';
  //fetch upcoming and past semesters from civi db
  $semester_tokens = array('upcoming' => 'All Upcoming Semesters');
  $upcoming_count = 0;
  $resource = db_query("SELECT label, value FROM {civicrm_option_value} WHERE option_group_id = 80 ORDER BY value");
  while ($row = db_fetch_object($resource)) {
    $token = strtolower(str_replace(' ', '-', $row->label));
    $semester_token_vals[$token] = $row->value;
    $semester_tokens[$token] = $row->label;
    if (strtotime(substr($row->value, 0, 10)) > time()) {
      ++$upcoming_count;
    }
  }
  // Warning if less than 4 semesters
  if ($upcoming_count < 4) {
    drupal_set_message("Only $upcoming_count upcoming semester" . ($upcoming_count == 1 ? ' is' : 's are') . " defined." . ' <a href="/staff/admissions/dates">Please click here to add more.</a>', $upcoming_count < 2 ? 'error' : 'warning');
  }
  // case status values
  $stats = ws_apply_case_status_options();

  $status = woolman_aval($_GET, 'status', '1-2-3');
  $pager = '';
  if ($arg != 'upcoming') {
    //validate arg
    if (!isset($semester_tokens[$arg])) {
      return drupal_not_found();
    }
    $semester = $semester_token_vals[$arg];

    //create pager
    $pages = array_keys($semester_tokens);
    $page = array_search($arg, $pages);
    $q = '?status=' . $status;
    $pager = '<div id="application-pager">';
    if ($page > 1) {
      $pager .= '<div id="pager-left"><a href="'. $pages[$page-1]. $q .'"><span> </span>' . $semester_tokens[$pages[$page-1]] .'</a></div>';
    }
    if ($page + 1 < count($pages)) {
      $pager .= '<div id="pager-right"><a href="'. $pages[$page+1]. $q .'">'. $semester_tokens[$pages[$page+1]] .'<span> </span></a></div>';
    }
    $pager .= '</div>';
  }

  $cases = ws_apply_staff_get_cases(array(
    'status_id' => str_replace('-', ',', $status),
    'semester' => $semester,
  ));

  //Empty results message
  if (!$cases) {
    $message = '<div class="app-report-container"><h3>' . $semester_tokens[$arg] . '</h3><p>There are no applicants for ' . $semester_tokens[$arg];
    if (!empty($status)) {
      $message .= ' with the status ';
      $s = explode('-', $status);
      $i = 1;
      foreach ($s as $st) {
        $message .= $stats[$st];
        if ($i++ < count($s)) {
          $message .= ' or ';
        }
      }
    }
    $message .= '.</p></div><div id="application-pager">' . $pager.'</div>';
    return drupal_get_form('_ws_apply_form_report', $semester_tokens) . $message;
  }

  ws_apply_get_case_activities($cases);

  // JS data for in-place editing and live updates
  $setting = array(
    'displayStatus' => explode('-', $status),
    'status' => $stats,
    'semester' => $semester,
    'timeStamp' => woolman_civi_log_latest(),
    'actTypes' => woolman_get_civi_options('activity_type'),
  );
  drupal_add_js(array('ws_staff' => $setting), 'setting');

  // Here's our table header
  $header = array(
    '#',
    array('data' => 'Name', 'class' => 'header headerSortDown'),
    'Age',
    'Grad Date',
    'Sem',
    'ST',
    'Parents',
    'School',
    'Status',
    'App Started',
    'Last Update',
  );
  foreach (array_keys(ws_application_steps()) as $step) {
    $pos = strpos($step, ' ') + 1;
    $abbr = $step[0] . strtoupper($step[$pos]);
    $header[] = array('data' => $abbr, 'title' => $step);
  }
  $header[] = 'Follow-Up';

  // Table row for each student
  $table = array();
  foreach ($cases as $case) {
    $row = ws_apply_staff_format_row($case);
    // Put count in first column
    $row['data'][0]['data'] = count($table[$case['semester']]) + 1;
    $table[$case['semester']][] = $row;
  }

  // display multiple semesters in the correct order
  ksort($table);

  $output = drupal_get_form('_ws_apply_form_report', $semester_tokens);

  foreach ($table as $semester => $rows) {
    $caption = woolman_get_semester($semester);
    $output .= '<div class="app-report-container">';
    $output .= theme('table', $header, $rows, array('class' => 'app-report-table tablesorter', 'data-semester' => $semester), $caption);
    $output .= '</div>';
  }
  return '
    <div id="staff-msg">
      <p id="busy-msg">Loading...</p>
    </div>' .
    $output . $pager .
    '<div id="view-activity" style="display:none;">
        <div id="activity-content""></div>
    </div>';
}

/**
 * Form to control reports
 */
function _ws_apply_form_report($form_state, $semesters) {
  if (!empty($_GET['status'])) {
    $default_stats = explode('-', $_GET['status']);
  }
  else {
    $default_stats = array(1, 2, 3);
  }

  return array(
    'semester' => array(
      '#type' => 'select',
      '#title' => 'Show Applicants for',
      '#prefix' => '<div class="subset">',
      '#options' => $semesters,
      '#default_value' => arg(3),
    ),
    'status' => array(
      '#type' => 'checkboxes',
      '#title' => 'Show Applications with Status',
      '#options' => ws_apply_case_status_options(),
      '#default_value' => $default_stats,
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => 'Go',
      '#prefix' => '<div id="submit-wrapper" class="form-item"><label>&nbsp;</label>',
      '#suffix' => '</div></div>',
    ),
  );
}

/**
 * Submit callback for report form
 */
function _ws_apply_form_report_submit($form, $form_state) {
  $vals = $form_state['values'];

  foreach ($form_state['values']['status'] as $stat=> $selected) {
    if (!$selected) {
      unset($vals['status'][$stat]);
    }
  }
  if (!empty($vals['status'])) {
    sort($vals['status']);
    $q['status'] = implode('-', $vals['status']);
  }

  if ($vals['semester'] != 'upcoming') {
    $sem = '/' . $vals['semester'];
  }
  drupal_goto('staff/admissions/applications' . $sem, $q) ;
}

/**
 * Format date for staff pages
 */
function ws_staff_date($date) {
  if ($date) {
    return '<span class="abs-dt">' . date('n/j/y', strtotime($date)) . '</span><span class="rel-dt"></span>';
  }
  return '';
}

/**
 * Fetch one or more cases
 */
function ws_apply_staff_get_cases($params) {
  $cases = $args = array();
  $query = "
    SELECT cas.id AS case_id, cas.start_date, act.id AS act_id, con.first_name, con.last_name, con.nick_name,
    con.birth_date, con.id AS contact_id, cas.status_id, sem.semesters_5, ai.hs_graduation_date_4 AS grad, ad.street_address, ad.city, ad.state_province_id, ad.country_id
    FROM {civicrm_case} cas
    INNER JOIN {civicrm_case_contact} cc ON cc.case_id = cas.id
    INNER JOIN {civicrm_contact} con ON con.id = cc.contact_id
    LEFT JOIN {civicrm_address} ad ON ad.contact_id = cc.contact_id AND ad.is_primary = 1
    LEFT JOIN {civicrm_value_additional_info_2} ai ON ai.entity_id = cc.contact_id
    LEFT JOIN {civicrm_activity} act ON act.id IN (SELECT activity_id FROM {civicrm_case_activity} WHERE case_id = cas.id) AND act.is_current_revision = 1 AND act.is_deleted = 0 AND act.activity_type_id = 47
    LEFT JOIN {civicrm_value_semester_choices_3} sem ON sem.entity_id = act.id
    WHERE (cas.is_deleted <> 1 AND con.is_deleted <> 1";

  if ($params['semester'] == 'upcoming') {
    $sems = array_keys(woolman_upcoming_semesters());
    $query .= " AND (sem.semesters_5 LIKE '%". implode("%' OR sem.semesters_5 LIKE '%", $sems) ."%' OR sem.semesters_5 IS NULL)";
  }
  else {
    $sems = array($params['semester']);
    $query .= " AND sem.semesters_5 LIKE '%{$params['semester']}%'";
  }

  if (!empty($params['status_id'])) {
    $query .= ' AND cas.status_id IN (' . $params['status_id'] . ')';
  }

  if (isset($params['case_id'])) {
    $query .= " AND cas.id IN (" . implode(',', $params['case_id']) . ")";
  }
  else {
    $query .= " AND cas.case_type_id LIKE '%2%'";
  }
  $query .= ")";

  if (!empty($params['force_case_id']) && is_numeric($params['force_case_id'])) {
    $query .= " OR cas.id = " . $params['force_case_id'];
  }
  $query .= " ORDER BY con.sort_name";

  $resource = db_query($query, $args);
  while ($row = db_fetch_object($resource)) {
    $case = array(
      'case_id' => $row->case_id,
      'contact_id' => $row->contact_id,
      'status_id' => $row->status_id,
      'nick_name' => $row->nick_name ? $row->nick_name : $row->first_name,
      'last_name' => $row->last_name,
      'display_name' => $row->first_name . '<i></i>' . ($row->nick_name ? ' (' . $row->nick_name . ')' : '') . ' <i></i>' . $row->last_name,
      'start_date' => $row->start_date,
      'recent_act' => $row->start_date,
      'birth_date' => $row->birth_date,
      'grad' => $row->grad,
      'semester_choices' => array(),
      'notes' => array(),
      'address' => woolman_format_address($row, "\n"),
      'st' => !empty($row->state_province_id) ? CRM_Core_PseudoConstant::stateProvinceAbbreviation($row->state_province_id) : '',
      'school' => array(),
      'parent' => array(),
      'semester' => '',
    );

    // Semester choices
    $sem_link = '<a href="#" title="Edit semester preference" data-atype="47" data-fn="createActivity">';
    if (!empty($row->semesters_5)) {
      $choices = woolman_multivalue_arr($row->semesters_5);
      sort($choices);
      foreach ($choices as $choice) {
        if (isset($sems) && empty($case['semester']) && in_array($choice, $sems)) {
          $case['semester'] = $choice;
        }
        $case['semester_choices'][] = $sem_link . str_replace('Spring', 'Spr', woolman_get_semester($choice)) . '</a>';
      }
    }
    else {
      $case['semester_choices'] = array($sem_link . '<em>None</em></a>');
    }
    if (empty($row->semesters_5) || $params['semester'] != 'upcoming') {
      $case['semester'] = $params['semester'];
    }

    $cases[$row->case_id] = $case;
  }

  if ($cases) {
    // Fetch related contacts
    $rel = array(1 => 'parent', 10 => 'school');
    $query = "SELECT cc.case_id, con.display_name, rel.contact_id_b AS contact_id, rel.relationship_type_id
      FROM {civicrm_case_contact} cc
      INNER JOIN {civicrm_relationship} rel ON rel.contact_id_a = cc.contact_id AND rel.relationship_type_id IN (" . implode(',', array_keys($rel)) . ") AND rel.contact_id_b <> " . WOOLMAN . "
      INNER JOIN {civicrm_contact} con ON con.id = rel.contact_id_b
      WHERE rel.is_active = 1 AND con.is_deleted = 0 AND cc.case_id IN (" . implode(',', array_keys($cases)) . ")
    ";
    $resource = db_query($query);
    while ($row = db_fetch_object($resource)) {
      $cases[$row->case_id][$rel[$row->relationship_type_id]][] = woolman_contact_link($row->display_name, $row->contact_id, array('attributes' => array('title' => 'View in CiviCRM')));
    }
  }
  return $cases;
}

/**
 * Fetch acts and add them to cases
 */
function ws_apply_get_case_activities(&$cases) {
  if ($cases) {
    $no_display = array(9,13,15,16,43,44,45,46,47);
    $now = date('Y-m-d H:i:s');
    $query = "SELECT ca.case_id, act.activity_type_id, act.id, act.subject, act.activity_date_time, ov.name AS status, av.label AS type, act.source_contact_id AS contact_id
      FROM  {civicrm_activity} act
      INNER JOIN  {civicrm_case_activity} ca ON ca.activity_id = act.id
      LEFT JOIN {civicrm_option_value} av ON av.option_group_id = 2 AND av.value = act.activity_type_id
      LEFT JOIN  {civicrm_option_value} ov ON ov.option_group_id = 25 AND ov.value = act.status_id
      WHERE act.is_current_revision = 1 AND act.is_deleted = 0
      AND ca.case_id IN (" . implode(',', array_keys($cases)) . ")
      ORDER BY act.activity_date_time";
    $resource = db_query($query);
    while ($row = db_fetch_object($resource)) {
      $case = $row->case_id;
      // Update most recent act
      if ($row->activity_date_time > $cases[$case]['recent_act'] && $row->activity_date_time <= $now) {
        $cases[$case]['recent_act'] = $row->activity_date_time;
      }
      // Add activities (except the ones we don't care about)
      if (!in_array($row->activity_type_id, $no_display)) {
        // Put standard activities in thier place
        if (in_array($row->activity_type_id, ws_application_steps())) {
          $cases[$case]['acts'][$row->activity_type_id][] = (array) $row;
        }
        // Other activities get added as notes
        else {
          $cases[$case]['notes'][] = (array) $row;
        }
      }
    }
  }
}

/**
 * Format table row for a case
 */
function ws_apply_staff_format_row($case) {
  $stats = ws_apply_case_status_options();

  if ($case['birth_date']) {
    $date = strtotime($case['birth_date']);
    $age = (int) ((time()-$date)/60/60/24/365.25);
    $case['birth_date'] = array('data' => $age, 'title' => 'DOB: ' . date('n/j/Y', $date));
  }
  if ($case['grad']) {
    $date = strtotime($case['grad']);
    $case['grad'] = array('data' => date('Y', $date), 'title' => 'Graduates ' . date('F Y', $date));
  }
  $row = array(
    array('data' => ''),
    woolman_contact_link($case['display_name'], $case['contact_id'], array('attributes' => array('title' => 'View in CiviCRM'))),
    $case['birth_date'],
    $case['grad'],
    implode(',<br />', $case['semester_choices']),
    array('data' => $case['st'], 'title' => $case['address']),
    implode(' &amp; ', $case['parent']),
    implode(' &amp; ', $case['school']),
    '<div class="sel" data-type="status" data-id="' . $case['status_id'] . '">' . $stats[$case['status_id']] .'</div>',
    ws_staff_date($case['start_date']),
    ws_staff_date($case['recent_act']),
  );

  $editable = FALSE;
  $prev = $i = 0;
  foreach (ws_application_steps() as $step => $type) {
    $cell = array('class' => 'cell ', 'data' => '<span class="icn"> </span>');
    $aid = '';
    $link = 'createActivity';
    // If doing more than one of this activity, increment array key
    $i = $type == $prev ? $i + 1 : 0;
    $prev = $type;
    if ($step == 'Transcript') {
      $editable = TRUE;
    }
    $cell['title'] = $step . ' for ' . $case['nick_name'] . ' ' . $case['last_name'] . " \n(not started)";

    //activity doesn't exist
    if (!($act = woolman_aval($case, "acts:$type:$i"))) {
      $cell['class'] .= 'ready';
    }
    //activity exists: provide appropriate links & titles
    else {
      $aid = ' data-aid="' . $act['id'] . '"';
      $cell['class'] .= strtolower(str_replace(' ', '-', $act['status']));
      if ($act['status'] != 'Not Started') {
        $cell['data'] .= ws_staff_date($act['activity_date_time']);
        if ($act['status'] == 'Completed' || !$editable) {
          $link = 'viewActivity';
        }
        switch($act['status']) {
          case 'Scheduled':
            $verb = 'scheduled for';
            break;
          case 'In Progress':
            $verb = $editable ? 'last updated' : 'draft saved';
            break;
          case 'Waiting':
            $verb = 'sent';
            break;
          default:
            $verb = strtolower($act['status']);
        }
        $cell['title'] = ($act['subject'] ? $act['subject'] : $step) . ' by ' . woolman_name('full', $act['contact_id']) . " \n(" . $verb . ' ' ._ws_apply_act_date_format($act, TRUE).')';
      }
      else {
        $cell['class'] .= ' ready';
      }



    }
    $cell['data'] = '<a href="#"' . $aid . ' data-atype="'. $type .'" data-fn="' . $link . '">' . $cell['data'] . '</a>';
    $row[] = $cell;
  }
  $notes = array();
  foreach ($case['notes'] as $act) {
    $link = $act['status'] == 'Completed' ? 'viewActivity' : 'createActivity';
    $verb = $act['status'] == 'Scheduled' ? 'scheduled for' : strtolower($act['status']);
    $notes[] = '<a href="#" data-aid="' . $act['id'] . '" data-atype="' . $act['activity_type_id'] . '" data-fn="' . $link . '" title="' . $act['type'] . ' by ' . woolman_name('full', $act['contact_id']) . " \n(" . $verb . ' ' . _ws_apply_act_date_format($act, TRUE) . ')">' . ws_staff_date($act['activity_date_time']) . ': ' . ($act['subject'] ? $act['subject'] : $act['type']) . '</a>';
  }
  // Notes cell
  $row[] = '<div class="notes">' . implode('<br/ >', $notes) . '<a href="#" class="new-note icon" data-fn="actionMenu"></a></div>';

  return array(
    'data' => $row,
    'data-case-id' => $case['case_id'],
    'data-contact-id' => $case['contact_id'],
    'class' => strtolower(str_replace(' ', '-', $stats[$case['status_id']]))
  );
}

/**
 * Serve AJAX requests for the staff pages
 */
function ws_admissions_js() {
  $output = '';
  if (!empty($_GET['op'])) {

    switch ($_GET['op']) {

      case 'status':
        if (empty($_GET['case_id']) || empty($_GET['selected']) || empty($_GET['contact_id']) ||
        !is_numeric($_GET['case_id']) || !is_numeric($_GET['selected']) || !is_numeric($_GET['contact_id'])) {
          break;
        }
        $result = woolman_civicrm_api('case', 'create', array(
          'id' => $_GET['case_id'],
          'status_id' => $_GET['selected'],
        ));
        if (!empty($result['is_error'])) {
          exit('"error"');
        }
        $stats = woolman_get_civi_options('case_status');
        woolman_civicrm_api('activity', 'create', array(
          'case_id' => $_GET['case_id'],
          'status_id' => 2,
          'source_contact_id' => woolman_user_cid(),
          'target_contact_id' => $_GET['contact_id'],
          'activity_type_id' => 16,
          'subject' => 'Case status changed to ' . $stats[$_GET['selected']],
        ));
        woolman_student_relationship($_GET['contact_id'], $_GET['selected'] == 2 ? 'create' : 'delete');
        ws_staff_js_update($_GET['ts'], $_GET['sem'], $_GET['case_id']);
        break;

      case 'activity_form':
        if (empty($_GET['case_id']) || empty($_GET['atype']) || empty($_GET['contact_id']) ||
        !is_numeric($_GET['case_id']) || !is_numeric($_GET['atype']) || !is_numeric($_GET['contact_id'])) {
          break;
        }
        $act = array();

        // Fetch activity
        if (!empty($_GET['aid']) && is_numeric($_GET['aid'])) {
          $act = ws_admissions_fetch_current_act($_GET['aid']);
        }
        // Search for existing activity if only one of this type is allowed in the case
        if (!$act) {
          $steps = array_flip(ws_application_steps()) + array(47 => 'Semester Pref');
          unset($steps[39]);
          if (isset($steps[$_GET['atype']])) {
            $query = db_query('SELECT * FROM {civicrm_activity}
              WHERE activity_type_id = %d AND is_deleted = 0 AND is_current_revision = 1
              AND id IN (SELECT activity_id FROM {civicrm_case_activity} WHERE case_id = %d)',
              $_GET['atype'], $_GET['case_id']);
            $act = db_fetch_array($query);
          }
        }

        $output = drupal_get_form('ws_staff_activity_create', $act, $_GET['atype'], $_GET['contact_id'], $_GET['case_id']);
        exit($output);
        break;

      case 'activity_form_submit':
        ws_staff_activity_create_submit(NULL, NULL);
        break;

      case 'update_check':
        ws_staff_js_update($_GET['ts'], $_GET['sem']);
        break;
    }
  }
  exit(json_encode($output));
}

/**
 * Form for creating/editing a activity
 */
function ws_staff_activity_create(&$form_state, $act, $activity_type_id, $cid, $case_id) {
  $form = array();
  // Semester pref gets a special form
  if ($activity_type_id == 47) {
    $choices = array();
    if ($act) {
      $result = woolman_civicrm_api('custom_value', 'get', array('entity_id' => $act['id'], 'return_custom_5' => 1));
      $choices = woolman_aval($result, 'values:5:0', array());
    }
    $form['semester'] = array(
      '#type' => 'checkboxes',
      '#prefix' => '<h3>Choose Semester(s):</h3>',
      '#options' => woolman_upcoming_semesters(5),
      '#default_value' => array_values($choices),
    );
  }
  // All other act types get this form
  else {
    $form['params_subject'] = array(
      '#type' => 'textfield',
      '#title' => 'Subject',
      '#size' => 70,
      '#default_value' => woolman_aval($act, 'subject'),
    );
    $js = "$('[name=params_activity_date_time]').val('" . date('m/d/Y') . "');$('[name=activity_time]').val('" . date('h:i a') . "'); return false;";
    $form['params_activity_date_time'] = array(
      '#type' => 'textfield',
      '#title' => 'Date',
      '#size' => 15,
      '#attributes' => array('class' => 'datepicker'),
      '#default_value' => date('m/d/Y', $act ? strtotime($act['activity_date_time']) : time()),
      '#prefix' => '<div class="subset">',
    );
    $form['activity_time'] = array(
      '#type' => 'textfield',
      '#title' => 'Time',
      '#size' => 10,
      '#default_value' => date('h:i a', $act ? strtotime($act['activity_date_time']) : time()),
      '#suffix' => '<div class="form-item now"><label>&nbsp;</label><button onclick="' . $js . '">Use Current Date/Time</button></div></div>',
    );

    $form['params_status_id'] = array(
      '#type' => 'select',
      '#title' => 'Status',
      '#options' => array('' => '- Select -') + woolman_get_civi_options('activity_status'),
      '#req' => 1,
      '#default_value' => woolman_aval($act, 'status_id', (in_array($_GET['atype'], ws_application_steps()) ? NULL : 2)),
      '#prefix' => '<div class="subset">',
    );
    $form['params_medium_id'] = array(
      '#type' => 'select',
      '#title' => 'Medium',
      '#options' => array('' => '- Select -') + woolman_get_civi_options('encounter_medium'),
      '#req' => 1,
      '#default_value' => woolman_aval($act, 'medium_id'),
      '#suffix' => $activity_type_id == 51 ? NULL : '</div>',
    );
    // Financial aid custom field
    if ($activity_type_id == 51) {
      $custom = $act ? woolman_civicrm_api('activity', 'get', array('return.custom_61' => 1, 'id' => $act['id']), 'single') : array();
      $form['params_custom_61'] = array(
        '#type' => 'radios',
        '#title' => 'Applying for Financial Aid?',
        '#options' => array('No', 'Yes'),
        '#default_value' => woolman_aval($custom, 'custom_61', NULL, TRUE),
        '#suffix' => '</div>',
      );
    }
    /* // Abandoned attempt to support file uploads
    $form['#attributes'] = array('enctype' => "multipart/form-data");
    $form['upload'] = array(
      '#type' => 'fieldset',
      '#title' => 'Attach Files',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#suffix' => '<div class="clear-block"> </div>',
    );
    for ($i = 1; $i < 4; ++$i) {
      $form['upload']['file_' . $i] = array(
        '#type' => 'file',
        '#title' => 'File ' . $i,
        '#size' => 20,
      );
    }*/
    $form['params_details'] = array(
      '#type' => 'textarea',
      '#title' => 'Details',
      '#resizable' => FALSE,
      '#id' => 'details_' . time(),
      '#default_value' => woolman_aval($act, 'details'),
    );
  }
  $hidden = array(
    'id' => woolman_aval($act, 'id', ''),
    'case_id' => $case_id,
    'target_contact_id' => $cid,
    'activity_type_id' => $activity_type_id
  );

  // Modify form for sending an email
  if ($activity_type_id == 3 && !$act) {
    unset($form['params_activity_date_time'], $form['activity_time'], $form['upload']);
    $hidden['status_id'] = 2;
    $hidden['medium_id'] = 3;
    $form['params_subject']['#mark_req'] = TRUE;
    $form['params_details']['#mark_req'] = TRUE;
    $form['params_details']['#resizable'] = TRUE;
    $form['params_details']['#title'] = 'Message';
    $form['params_details']['#rows'] = 10;
    foreach (array('bcc', 'cc', 'to') as $w => $to) {
      $form[$to] = array(
        '#type' => 'textfield',
        '#title' => ucfirst($to),
        '#weight' => -10 - $w,
        '#size' => 80,
        '#attributes' => array('class' => 'full-width'),
        '#autocomplete_path' => 'staff/js/autocomplete/email',
      );
    }
    $c = woolman_contact_get($cid);
    if (!empty($c['email'])) {
      $form['to']['#default_value'] = '"' . $c['nick_name'] . ' ' . $c['last_name'] . '" <' . $c['email'] . '>, ';
    }
    $email = $phone = '';
    foreach (woolman_civicrm_api('email', 'get', array('contact_id' => woolman_user_cid()), 'values') as $e) {
      if ($e['email'] && ($e['is_primary'] || !$email)) {
        $email = $e['email'];
      }
      if (strpos($e['email'], '@woolman.org')) {
        $email = $e['email'];
        break;
      }
    }
    foreach (woolman_civicrm_api('phone', 'get', array('contact_id' => woolman_user_cid()), 'values') as $e) {
      if ($e['phone'] && $e['location_type_id'] == 2) {
        $phone = "\n" . $e['phone'] . ($e['phone_ext'] ? ' x' . $e['phone_ext'] : '');
      }
    }
    $form['params_details']['#default_value'] = "\n\n\n" . woolman_name('full') . $phone . "\nThe Woolman Semester School\nLive What You Believe!";
    $email = '"' . woolman_name('full') . '" <' . $email . '>';
    $form['from'] = array('#type' => 'hidden', '#value' => $email);
    $form['from_display'] = array('#value' => '<strong>From: </strong>' . check_plain($email), '#weight' => -5);
  }

  // These hidden fields will automatically be added to the params by the submission handler
  foreach ($hidden as $k => $v) {
    $form['params_' . $k] = array('#type' => 'hidden', '#value' => $v);
  }
  // A non-ajax submit button (part of the abandoned file attempt)
  // $form['submit'] = array('#type' => 'submit', '#value' => 'Submit', '#attributes' => array('style' => 'display:none'));
  return $form;
}

/**
 * An un-Drupalish submit handler that ignores $form_state and uses data from $_POST
 * AJAX was easier this way :P
 */
function ws_staff_activity_create_submit($form, $form_state) {
  $params = array();
  foreach ($_POST as $key => $value) {
    if (substr($key, 0, 6) == 'params') {
      $params[substr($key, 7)] = $value;
    }
  }
  // Process semester pref
  if ($params['activity_type_id'] == 47) {
    $params['activity_date_time'] = 'now';
    $params['source_contact_id'] = woolman_user_cid();
    $params['custom_5'] = woolman_multivalue_str($_POST['semester']);
    $cid = $params['target_contact_id'];
    $case = _ws_apply_get_case($cid, 2, FALSE);
  }
  else {
    $params['activity_date_time'] .= ' ' . $_POST['activity_time'];
  }
  // Create
  if (!$params['id']) {
    unset($params['id']);
    $params['source_contact_id'] = woolman_user_cid();
  }
  // Update
  else {
    unset($params['target_contact_id']);
  }
  // Send email
  if (empty($params['id']) && $params['activity_type_id'] == 3) {
    $msg = array(
      'subject' => $params['subject'],
      'body' => $params['details'],
      'cc' => trim($_POST['cc']),
      'bcc' => trim($_POST['bcc']),
    );
    drupal_mail('woolman_website', 0, trim($_POST['to']), language_default(), $msg, $_POST['from']);
    $params['details'] = nl2br($params['details']);
  }

  // Save activity
  woolman_civicrm_api('activity', 'create', $params);

  // Update relationship dates for enrolled students
  if (!empty($case) && $case['status_id'] == 2) {
    woolman_student_relationship($cid, 'create');
  }
  if ($form || $form_state) {
    // For non-ajax submissions
    drupal_goto('staff/admissions/applications');
  }
  // For ajax submissions
  ws_staff_js_update($_GET['ts'], $_GET['sem'], $params['case_id']);
}

/**
 * Check the civicrm log for anything new and return updates via JSON
 */
function ws_staff_js_update($ts, $semester, $case_id = NULL) {
  $new_ts = woolman_civi_log_latest();
  $ret = array('ts' => $new_ts);
  if ($ts < $new_ts) {
    $log = $cases = array();
    $db = db_query("SELECT l.entity_id, l.modified_id, l.entity_table, l.data, cc.case_id AS contact_case_id, ca.case_id AS activity_case_id
      FROM {civicrm_log} l
      LEFT JOIN {civicrm_case_contact} cc ON l.entity_table = 'civicrm_contact' AND cc.contact_id = l.entity_id
      LEFT JOIN {civicrm_case_activity} ca ON l.entity_table = 'civicrm_activity' AND ca.activity_id = l.entity_id
      WHERE l.modified_date > '$ts' AND (ca.case_id IS NOT NULL OR cc.case_id IS NOT NULL) AND (
        ca.case_id IN (SELECT id FROM {civicrm_case} WHERE case_type_id LIKE '%2%' AND is_deleted = 0) OR
        cc.case_id IN (SELECT id FROM {civicrm_case} WHERE case_type_id LIKE '%2%' AND is_deleted = 0)
      )");
    while ($row = db_fetch_object($db)) {
      $id = $row->entity_table == 'civicrm_contact' ? $row->contact_case_id : $row->activity_case_id;
      // If a contact and activity are both updated, we display the activity
      if (empty($log[$id]) || $row->entity_table == 'civicrm_activity') {
        $log[$id] = array(
          'type' => $row->entity_table,
          'by' => $row->modified_id,
          'id' => $row->entity_id,
          'action' => strpos($row->data, 'create') ? 'added' : 'updated',
        );
      }
    }
    if ($log) {
      module_load_include('inc', 'woolman_website', 'woolman_website_utils');
      $cases = ws_apply_staff_get_cases(array(
        'case_id' => array_keys($log),
        'force_case_id' => $case_id,
        'semester' => $semester,
      ));
      ws_apply_get_case_activities($cases);

      foreach ($cases as $id => $case) {
        // Format status message based on entity type, user, and action
        if ($log[$id]['type'] == 'civicrm_contact') {
          $msg = 'Contact info updated';
        }
        else {
          $act = woolman_civicrm_api('activity', 'get', array('id' => $log[$id]['id']), 'single');
          $type = 'Activity';
          if ($type_id = woolman_aval($act, 'activity_type_id')) {
            $types = CRM_Core_PseudoConstant::activityType(TRUE, TRUE);
            $type = woolman_aval($types, $type_id, 'Activity');
          }
          if (substr($type, 0, 6) == 'Change') {
            $type = substr($type, 7);
            $log[$id]['action'] = 'changed';
          }
          if ($type == 'Email') {
            $log[$id]['action'] = str_replace('added', 'sent', $log[$id]['action']);
          }
          $msg = $type . ' ' . $log[$id]['action'];
        }
        $msg .= $case['contact_id'] == $log[$id]['by'] ? ' by ' : ' for ';
        $msg .= $case['nick_name'] . ' ' . $case['last_name'];
        $row = ws_apply_staff_format_row($case);
        $row['data'][0]['class'] = 'active';
        $ret['rows'][] = array(
          'semester' => $case['semester'],
          'status' => $case['status_id'],
          'case_id' => $id,
          'msg' => $msg,
          'row' => woolman_table_row($row),
        );
      }
    }
  }
  exit(json_encode($ret));
}
