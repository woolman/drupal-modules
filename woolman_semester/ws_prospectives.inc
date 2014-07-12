<?php

module_load_include('inc', 'woolman_semester', 'ws_apply_online_utils');

function ws_prospectives() {
  drupal_add_css('sites/all/libraries/jquery.ui/themes/default/ui.all.css', 'module', 'all', FALSE);
  drupal_add_css(drupal_get_path('module', 'woolman_semester') . '/css/ws_prospectives.css', 'module', 'all', FALSE);

  jquery_ui_add(array('ui.dialog'));
  drupal_add_js('sites/all/libraries/tablesorter/jquery.tablesorter.min.js', 'module', 'header', TRUE, FALSE);
  drupal_add_js(drupal_get_path('module', 'woolman_semester') . '/js/ws_prospectives.js', 'module', 'header', TRUE, FALSE);

  //get admissions reps (case managers)
  $reps = array('- Anyone -');
  $db = db_query(
   "SELECT first_name, nick_name, last_name, id FROM {civicrm_contact}
    WHERE id IN (SELECT contact_id_b from {civicrm_relationship}
    WHERE relationship_type_id = 13 AND is_active = 1 AND case_id IN
      (SELECT id FROM {civicrm_case} WHERE is_deleted = 0))
    ORDER BY sort_name"
  );
  while ($row = db_fetch_array($db)) {
    $firsts[$row['id']] = ($row['nick_name'] ? $row['nick_name'] : $row['first_name']);
    $lasts[$row['id']] = ' '.strtoupper(substr($row['last_name'], 0, 1)).'.';
  }
  // Append last initial to first name if 2 people have the same name
  foreach ($firsts as $id => $first) {
    if (count(array_keys($firsts, $first)) > 1) {
      $reps[$id] = $first . $lasts[$id];
    }
    else {
      $reps[$id] = $first;
    }
  }

  //get variables
  $stats = woolman_get_civi_options('case_status');
  $mediums = woolman_get_civi_options('encounter_medium');
  $states = array('- Any -') + woolman_get_civi_options('state_abbr');

  //get trip
  if (!empty($_GET['trip'])) {
    $db = db_query('SELECT id, subject, activity_date_time as date FROM {civicrm_activity} WHERE id = %d', $_GET['trip']);
    $trip = db_fetch_array($db);
    $trip['subject'] = trim(str_replace('Outreach Trip to', '', $trip['subject']));
  }

  if (count($_GET) == 1) {
    module_load_include('inc', 'woolman_semester', 'ws_prospectives_dashboard');
    return drupal_get_form('ws_admissions_prospective_controls', $reps, $stats, $states, $trip)
    . ws_prospectives_dashboard($reps);
  }

  return drupal_get_form('ws_admissions_prospective_controls', $reps, $stats, $states, $trip). drupal_get_form('ws_admissions_prospective_form', $reps, $stats, $states, $trip, $mediums);
}

function ws_admissions_prospective_form($form_state, $reps, $stats, $states, $trip, $mediums) {
  $self = woolman_contact_get();
  $loc = woolman_get_civi_options('location_type');
  $semesters = woolman_upcoming_semesters(6);
  if (($upcoming_count = count($semesters)) < 4) {
    drupal_set_message("Only $upcoming_count upcoming semester" . ($upcoming_count == 1 ? ' is' : 's are') . " defined." . ' <a href="/staff/admissions/dates">Please click here to add more.</a>', $upcoming_count < 2 ? 'error' : 'warning');
  }
  //filter by status
  if (!empty($_GET['status'])) {
    $status = str_replace('-', ',', $_GET['status']);
  }
  else {
    return array('empty' => array('#value' => '<b>Choose options from the list above to find prospective students.</b><br />NOTE: Too broad a search could yield thousands of results, which will take a long time to display.'));
  }

  //get cases
  $query = "SELECT cas.id AS case_id, cc.contact_id, cas.status_id, cas.start_date, con.first_name, con.last_name, con.nick_name, em.email, ph.phone, rel.contact_id_b AS rep_id, act.id as act_id, act.subject, grad.hs_graduation_date_4 AS grad, addr.state_province_id, addr.country_id, addr.city, addr.street_address, ph.location_type_id
    FROM  {civicrm_case} cas
    JOIN {civicrm_case_contact} cc ON cc.case_id = cas.id
    JOIN {civicrm_contact} con ON con.id = cc.contact_id
    LEFT JOIN {civicrm_address} addr ON addr.contact_id = cc.contact_id AND addr.is_primary = 1
    LEFT JOIN {civicrm_email} em ON em.contact_id = cc.contact_id AND em.is_primary = 1
    LEFT JOIN {civicrm_phone} ph ON ph.contact_id = cc.contact_id AND ph.is_primary = 1
    LEFT JOIN {civicrm_value_additional_info_2} grad ON grad.entity_id = cc.contact_id
    LEFT JOIN {civicrm_relationship} rel ON rel.case_id = cas.id AND rel.relationship_type_id = 13 AND rel.is_active = 1
    LEFT JOIN {civicrm_activity} act ON act.activity_type_id = 33 AND act.is_current_revision = 1 AND act.is_deleted = 0
    AND act.id IN (SELECT tar.activity_id FROM {civicrm_activity_contact} tar WHERE tar.contact_id = cc.contact_id AND tar.record_type_id = 3)
    WHERE cas.case_type_id = '1' AND cas.is_deleted = 0 AND cas.status_id IN ($status)";
  $args = array();

  //filter by staff rep
  if (!empty($_GET['rep'])) {
    $query .= ' AND rel.contact_id_b = ' . $_GET['rep'];
  }

  //filter by start date
  if (!empty($_GET['start'])) {
    list($op, $num, $unit) = explode('-', $_GET['start']);
    $date = date_format(date_create('now-' . $num . $unit), 'Y-m-d');
    $op = ($op == 'lt' ? '>=' : '<=');
    $query .= " AND cas.start_date $op '$date'";
  }

  //filter by latest activity
  if (!empty($_GET['act'])) {
    list($op, $num, $unit) = explode('-', $_GET['act']);
    $date = date_format(date_create('now-' . $num . $unit), 'Y-m-d');
    $op = ($op == 'lt' ? 'IN' : 'NOT IN');
    $query .= " AND cas.id $op
      (SELECT ca2.case_id FROM {civicrm_case_activity} ca2 WHERE ca2.activity_id IN
        (SELECT act2.id FROM {civicrm_activity} act2 WHERE act2.activity_date_time >= '$date' AND act2.is_deleted = 0 AND act2.is_current_revision = 1 AND act2.status_id IN (2,3))
      )";
  }

  //filter by pending activities
  if (!empty($_GET['pending'])) {
    if ($_GET['pending'] == 'overdue') {
      $op = 'lt';
      $date = date_format(date_create('now'), 'Y-m-d');
    }
    elseif ($_GET['pending'] == 'none') {
      $date_arg = "activity_date_time <= '$date' AND ";
    }
    else {
      list($op, $num, $unit) = explode('-', $_GET['pending']);
      $date = date_format(date_create('now+' . $num . $unit), 'Y-m-d');
    }
    if ($_GET['pending'] != 'none')
      $date_arg = "AND activity_date_time <= '$date'";
    $op = ($op == 'lt' ? 'IN' : 'NOT IN');
    $query .= " AND cas.id $op
      (SELECT ca3.case_id FROM {civicrm_case_activity} ca3 WHERE ca3.activity_id IN
        (SELECT act3.id FROM {civicrm_activity} act3 WHERE act3.is_deleted = 0 AND act3.is_current_revision = 1 AND act3.status_id NOT IN (2,3) $date_arg)
      )";
  }

  //filter by outreach trip
  if (!empty($_GET['trip'])) {
    $query .= ' AND act.id = ' . $_GET['trip'];
  }

  //filter by state
  if (!empty($_GET['state'])) {
    $query .= ' AND addr.state_province_id = ' . $_GET['state'];
  }

  //filter by school
  if (!empty($_GET['school'])) {
    $query .= ' AND con.id IN (SELECT contact_id_a FROM {civicrm_relationship} WHERE relationship_type_id = 10 AND contact_id_b = ' . $_GET['school'].')';
  }

  //filter by state
  if (!empty($_GET['name'])) {
    $query .= " AND (con.display_name LIKE '%%%s%%' OR con.sort_name LIKE '%%%s%%')";
    $args[] = $args[] = $_GET['name'];
  }

  $query .= ' ORDER BY con.sort_name';
  $db = db_query($query, $args);

  while ($row = db_fetch_array($db)) {
    $id = $row['case_id'].'-' . $row['contact_id'];
    if (!$form[$id]) {
      $form[$id] = array(
        'status' => array('#value' => $stats[$row['status_id']]),
        'start_date' => array('#value' => $row['start_date']),
        'latest_act' => array('#value' => $row['start_date']),
        'grad' => array('#value' => substr($row['grad'], 0, 4)),
        'completed' => array('#value' => ''),
        'pending' => array('#value' => ''),
        'completed_count' => array('#value' => 0),
        'pending_count' => array('#value' => 0),
        'reps' => array('#value' => $reps[$row['rep_id']]),
        'trip' => array('#value' => trim(str_replace('Outreach Trip to', '', $row['subject']))),
        'state' => array('#value' => woolman_aval($states, $row['state_province_id'], '')),
      );
      $address = woolman_format_address($row, "\n");
      //using non-printing html <i></i> as spacers between name components
      $form[$id]['name'] = array('#value' => '<div class="nm"><b>' . $row['first_name'].'<i></i>'.($row['nick_name'] ? ' "' . $row['nick_name'].'"' : '').' <i></i>' . $row['last_name'].'<i></i></b>
      <div class="h-l">
        <a target="_blank" title="view case" class="v-c" href="/civicrm/contact/view/case?reset=1&id=' . $row['case_id'].'&cid=' . $row['contact_id'].'&action=view"></a>
        <a target="_blank" title="view student" class="v-s" href="/civicrm/contact/view?reset=1&cid=' . $row['contact_id'].'"></a>
        <a target="_blank" title="edit student" class="e-s" href="/civicrm/contact/add?reset=1&action=update&context=search&cid=' . $row['contact_id'].'"></a>'.
        ($row['email'] ? '<a target="_blank" class="e-m" title="email ' . $row['email'].'" href="mailto:' . $row['email'].'"></a>' : '<span class="e-m" title="no email address"></span>').
        '<span class="p-n" title="'.($row['phone']?$loc[$row['location_type_id']].': ' . $row['phone']:'no phone number').'"></span>
        <span class="a-d" title="'.($address?$address:'no address').'"></span>
      </div></div>');

      $options[$id] = '';

      $case_ids .= ',' . $row['case_id'];
    }
    else {
      //handles more than one rep
      if (strpos($form[$id]['reps']['#value'], $reps[$row['rep_id']]) === FALSE)
        $form[$id]['reps']['#value'] .= ', ' . $reps[$row['rep_id']];
    }
  }

  //Empty results message
  if (!($case_ids = ltrim($case_ids, ','))) {
    $message = '<p>There are no prospective students';

    if ($_GET['state']) {
      $db = db_query('SELECT name FROM {civicrm_state_province} WHERE id = %d', $_GET['state']);
      $state=db_fetch_array($db);
      if (!($st=$state['name']))
        $st = 'error, unknown state';
      $message .= ' from ' . $st;
    }

    $s = explode(',', $status);
    if (count($s) < 4) {
      $message .= ' with the status ';
      $i = 1;
      foreach ($s as $st) {
        $message .= $stats[$st];
        if ($i++ < count($s))
          $message .= ' or ';
      }
    }

    if (!empty($_GET['rep']))
      $message .= ' with ' . $reps[$_GET['rep']].' as their admissions rep';

    if ($trip)
      $message .= ' from the ' . $trip['subject'];

    if (!empty($_GET['school'])) {
      $db = db_query('SELECT display_name FROM {civicrm_contact} WHERE id = %d', $_GET['school']);
      $school=db_fetch_array($db);
      if (!($sch=$school['display_name']))
        $sch = 'error, unknown school';
      $message .= ' who are students at ' . $sch;
    }

    if (!empty($_GET['act']) || !empty($_GET['start']) || !empty($_GET['pending']))
      $message .= ' within the specified timeframe';

    $message .= '.</p>';
    return array('empty' => array('#value' => $message));
  }

  //get case activities
  $query = "SELECT ca.case_id, cc.contact_id, act.activity_type_id, act.id, act.subject, act.activity_date_time, ov.name AS status, con.display_name, av.label AS type, sem.semesters_5 AS semesters
    FROM  {civicrm_activity} act
    JOIN  {civicrm_case_activity} ca ON ca.activity_id = act.id
    JOIN  {civicrm_case_contact} cc ON ca.case_id = cc.case_id
    JOIN  {civicrm_activity_contact} ac ON ac.activity_id = act.id AND ac.record_type_id = 2
    JOIN  {civicrm_contact} con ON ac.contact_id = con.id
    LEFT JOIN {civicrm_option_value} av ON av.option_group_id = 2 AND av.value = act.activity_type_id
    LEFT JOIN {civicrm_option_value} ov ON ov.option_group_id = 25 AND ov.value = act.status_id
    LEFT JOIN {civicrm_value_semester_choices_3} sem ON sem.entity_id = ca.activity_id
    WHERE act.is_current_revision = 1 AND act.is_deleted = 0
    AND ca.case_id IN ($case_ids)
    ORDER BY act.activity_date_time
    ";
  $db = db_query($query);

  $now = date_format(date_create('now'), 'Y-m-d');

  while ($row = db_fetch_array($db)) {
    $id = $row['case_id'].'-' . $row['contact_id'];

    if ($row['type'] == 'Semester Preference') {
      $choices = woolman_multivalue_arr($row['semesters']);
      foreach ($choices as $choice) {
        if ($form[$id]['semesters']['#value']) {
          $form[$id]['semesters']['#value'] .= ' or ';
        }
        $form[$id]['semesters']['#value'] .= woolman_get_semester($choice);
      }
    }

    else {
      $title = ($row['subject'] ? $row['subject'] : $row['type']);

      if ($row['status'] == 'Completed' || $row['status'] == 'Left Message') {
        ++$form[$id]['completed_count']['#value'];
        if ($form[$id]['completed']['#value'])
          $form[$id]['completed']['#value'] .= ', ';
        $form[$id]['completed']['#value'] .= '<a title="View Activity: ' . $row['type'].' by ' . $row['display_name'].' ('._ws_admissions_date_format($row).')" href="javascript:viewActivity(' . $row['id'].',' . $row['contact_id'].')">' . $title.'</a>';
        //update latest act date
        if ($form[$id]['latest_act']['#value'] < $row['activity_date_time'])
          $form[$id]['latest_act']['#value'] = $row['activity_date_time'];
      }
      else {
      ++$form[$id]['pending_count']['#value'];
        if ($form[$id]['pending']['#value'])
            $form[$id]['pending']['#value'] .= ', ';
        $form[$id]['pending']['#value'] .= '<a target="_blank" title="Update Activity: ' . $row['type'].' by ' . $row['display_name'].' ' . $row['status'].($row['status']=='Scheduled'?' for ':': ')._ws_admissions_date_format($row).'" class="'.($row['activity_date_time']<$now ? 'o-d' : 'o-k').'" href="/civicrm/case/activity?action=update&reset=1&cid=' . $row['contact_id'].'&caseid=' . $row['case_id'].'&id=' . $row['id'].'">' . $title.'</a>';
      }
    }
  }
  $form['cases'] = array(
    '#type' => 'checkboxes',
    '#options' => $options,
  );
  $form['actions'] = array(
    '#type' => 'select',
    '#title' => 'Actions',
    '#options' => array(
      'Choose An Action:',
      'followup' => 'Schedule Follow-Up',
      'email' => 'Send Email',
      'left-msg' => 'Record Activity: "Left Message"',
      'act-14' => 'Record Activity: "Followed-Up"',
      'act-29' => 'Record Activity: "Sent More Info"',
      'act-28' => 'Record Activity: "Communication from Student"',
      'act-31' => 'Record Activity: "Facebook Friendship Confirmed"',
      'status' => 'Change Case Status',
      'rep' => 'Change Admissions Rep',
      'grad' => 'Change Graduation Date',
      'sem' => 'Change Semester Preference',
    ),
  );
  //bulk actions params
  $y = date('Y'); $y -= 2;
  for ($i=$y; $i<=$y+6; $i++) {
    $grad_dates[$i] = $i;
  }
  $times = array('000000' => 'Midnight');
  for ($t=1; $t<24; $t++) {
    $times[ ($t<10 ? '0' : '') . $t . '0000' ] = ($t<13 ? $t : $t-12) . ':00 ' . ($t<12 ? 'am' : 'pm');
  }

  unset($reps[0]);
  $reps[$self['contact_id']] = $self['display_name'];

  $form['params']['change_status'] = array(
    '#type' => 'select',
    '#title' => 'Status',
    '#options' => array(
      3 => 'Needs Follow-Up',
      1 => 'In Progress',
      4 => 'Withdrawn',
      2 => 'Complete',
    ),
  );
  $form['params']['change_rep'] = array(
    '#type' => 'select',
    '#title' => 'Assign To',
    '#options' => $reps,
    '#default_value' => $self['contact_id'],
  );
  $form['params']['grad'] = array(
    '#type' => 'select',
    '#title' => 'Grad Date',
    '#options' => $grad_dates,
  );
  $form['params']['semester'] = array(
    '#type' => 'checkboxes',
    '#title' => 'Semesters',
    '#options' => $semesters,
  );
  $form['params']['date'] = array(
    '#type' => 'date',
    '#title' => 'Date',
  );
  $form['params']['time'] = array('#type' => 'select',
    '#title' => 'Time',
    '#options' => $times,
    '#default_value' => date('H').'0000',
  );
  $form['params']['subject'] = array(
    '#type' => 'textfield',
    '#title' => 'Subject',
    '#default_value' => 'Hi {nickname}',
    '#description' => '(Optional)',
  );
  $form['params']['medium'] = array(
    '#type' => 'select',
    '#title' => 'Communication Method',
    '#options' => $mediums,
    '#default_value' => 2,
  );
  $form['params']['details'] = array(
    '#type' => 'textarea',
    '#title' => 'Details',
    '#default_value' => "Hi {nickname},\n",
    '#rows' => 7,
    '#cols' => 80,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Make Changes',
  );
  if (!empty($_GET['emailprompt'])) {
    $form['cases']['#default_value'] = array_keys($options);
    $form['actions']['#default_value'] = 'email';

    $interval = format_interval(time() - strtotime($trip['date']), 1);
    $time_ago = $interval.' ago';
    if (strpos($interval, 'hour') || strpos($interval, 'second') || strpos($interval, 'minute'))
      $time_ago = 'today';
    if (strpos($interval, 'day'))
      $time_ago = 'this week';
    if ($interval=='1 day')
      $time_ago = 'yesterday';
    if ($interval=='1 week')
      $time_ago = 'last week';

    $form['params']['subject']['#default_value'] = 'Good meeting you {nickname}';
    $form['params']['details']['#default_value'] = "Hi {nickname}, it was great meeting you at " . $trip['subject']." $time_ago, and I'm excited to talk to you more about the Woolman Semester.\n
If you like what you've heard so far about Woolman and think that this is the education and experience you want for yourself, I encourage you to begin the application process soon to reserve a space in our program. You can come as a Junior, Senior, of Post-grad student.\n
Woolman is a transformative experience for our students. Our staff and teachers help students realized that they have the power to effect change in the world on their own terms, and we offer them the support of a community where teachers, staff, their families, and students have all come to do the work of peace, social justice, and sustainable living. I hope that you will feel inspired to pursue Woolman as an educational opportunity and join us in this work.\n
While you're still getting to know us there is great information on our website, http://semester.woolman.org. You can also get in touch with current staff and students on our Facebook page, http://facebook.com/woolmansemester, and there are great examples of what our students are up to at http://blog.woolman.org. I hope you like what you find when you explore these sites!\n
Please call or email me anytime to learn more about our school. I am here to help answer questions and guide you through the inquiry process. Is there anything you'd like to know about us now?\n
I hope to hear from you soon!\n
Best Regards,
{$self['nick_name']} {$self['last_name']}
Woolman Semester Admissions
http://semester.woolman.org
";
    $phones = woolman_civicrm_api('phone', 'get', array('contact_id' => $self['contact_id']));
    foreach (woolman_aval($phones, 'values', array()) as $ph) {
      if ($ph['location_type_id'] == 2) {
        $form['params']['details']['#default_value'] .= $ph['phone'];
        if (!empty($ph['phone_ext'])) {
          $form['params']['details']['#default_value'] .= ' x' . $ph['phone_ext'];
        }
        break;
      }
    }
    $form['params']['details']['#rows'] = 13;
    $form['params']['details']['#cols'] = 95;
  }
  $form['#suffix'] = '<div id="view-activity" style="display:none">
                      <div id="activity-content"></div>
                    </div>
                    <div id="email-confirm-box" style="display:none">
                      <div id="email-confirm-box-inner">
                        <em>This message will be sent to <span id="email-dialog-count"></span>, with individual names substituted for each one. For example:</em>
                        <p><b>From: </b>' . $self['nick_name'] . ' ' . $self['last_name'] . ' &lt;' . $self['email'].'&gt;</p>
                        <p><b>To: </b><span id="email-dialog-to"></span></p>
                        <p><b>Subject: </b><span id="email-dialog-subject"></span></p>
                        <p class="underline"><b>Message: </b><div id="email-dialog-body"></div></p>
                      </div>
                    </div>';
  return $form;
}

function theme_ws_admissions_prospective_form($form) {

  if (array_key_exists('empty', $form)) {
    return $form['empty']['#value'];
  }

  $header = array(
    array('data' => '<span>&nbsp;</span>', 'class' => 'checkbox', 'title' => 'Check/uncheck all'),
    array('data' => '<span></span>', 'class' => 's', 'title' => 'Sort by status'),
    array('data' => 'Student', 'class' => 'header headerSortDown', 'title' => 'Sort by last name'),
    array('data' => 'St.<br />&nbsp;', 'title' => 'Sort by State'),
    array('data' => 'HS<br />Grad.&nbsp;&nbsp;', 'title' => 'Sort by HS graduation date'),
    array('data' => 'Sem.<br />Pref.&nbsp;&nbsp;', 'title' => 'Sort by semester preference'),
    array('data' => 'Staff&nbsp;&nbsp;<br />Rep.', 'title' => 'Sort by case manager'),
    array('data' => 'Start<br />Date&nbsp;&nbsp;&nbsp;', 'title' => 'Sort by case start date'),
    array('data' => 'Trip', 'title' => 'Sort by outreach trip'),
    array('data' => 'Latest<br />Activity&nbsp;&nbsp;', 'title' => 'Sort by latest activity date'),
    array('data' => '# of Completed Activities', 'title' => 'Sort by # of Completed Activities'),
    array('data' => '# of Pending Activities', 'title' => 'Sort by # of Pending Activities')
  );

  $rows = array();
  $c = 0;
  foreach (element_children($form) as $key) {
    if (isset($form[$key]['name'])) {
      switch($form[$key]['status']['#value']) {
        case 'Needs Follow-Up': $stat=1; break;
        case 'In Progress': $stat=2; break;
        case 'Successfully Complete': $stat=3; break;
        case 'Withdrawn': $stat=4;
      }
      $row = array();
      $row[] = array('data' => drupal_render($form['cases'][$key]), 'class' => 'checkbox');
      $row[] = array('data' => '<span>' . $stat.'</span>', 'class' => 's s' . $stat, 'title' => drupal_render($form[$key]['status']));
      $row[] = array('data' => drupal_render($form[$key]['name']));
      $row[] = array('data' => drupal_render($form[$key]['state']));
      $row[] = array('data' => drupal_render($form[$key]['grad']));
      $row[] = array('data' => drupal_render($form[$key]['semesters']));
      $row[] = array('data' => drupal_render($form[$key]['reps']));
      $row[] = array('data' => _ws_admissions_date_format($form[$key]['start_date']['#value']));
      drupal_render($form[$key]['start_date']);
      $row[] = array('data' => drupal_render($form[$key]['trip']));
      $row[] = array('data' => _ws_admissions_date_format($form[$key]['latest_act']['#value']));
      drupal_render($form[$key]['latest_act']);
      $completed = drupal_render($form[$key]['completed_count']);
      $row[] = array('data' => $completed.($completed == 1? ' activity: ' : ' activities: ').drupal_render($form[$key]['completed']));
      $pending = drupal_render($form[$key]['pending_count']);

      //provide add link if no pending activities
      unset($add_new);
      if (!$pending) {
        list($case_id, $contact_id) = explode('-', $key);
        $add_new = '<a class="new-note icon" title="Add follow-up activity" target="_blank" href="/civicrm/case/activity?action=add&reset=1&cid=' . $contact_id.'&caseid=' . $case_id.'&atype=14"> </a>';
      }
      $row[] = array('data' => $pending.($pending == 1? ' activity: ' : ' activities: ').drupal_render($form[$key]['pending']) . $add_new);

      $rows[] = $row;
      ++$c;
    }
  }

  $output .= '<div id="bulk-actions">'.drupal_render($form['actions']).'<div id="message-tokens" class="status description" style="display:none"><b>Name tokens:</b><br>{nickname} -- replaced with the persons\'s nickname, if they have one, or else their first name<br>{firstname} -- replaced with the person\'s official first name<br>{lastname} -- the person\'s last name</div>'.drupal_render($form['params']).'</div><div id="make-changes-wrapper"> '. drupal_render($form['submit']).'</div>';

  $output .= '<div id="progresscontainer"><div id="progressbar" style="display:none">Sorting...</div></div><div id="tableinfo">' . $c.' record'.($c==1?'':'s').' found. Click headers to sort table, shift-click to sort by more than one column.</div>';

  $output .= theme('table', $header, $rows, array('id' => 'prospectives', 'class' => 'tablesorter'));
  $output .= drupal_render($form);

  return $output;
}

function ws_admissions_prospective_form_validate($form, $form_state) {
  $vals = $form_state['values'];
  $val = TRUE;

  if (!array_search(TRUE, $vals['cases'])) {
    form_set_error('cases', 'You must select at least one student.');
    $val = FALSE;
  }

  switch($vals['actions']) {
    case '0':
      form_set_error('actions', 'Choose an action');
      $val = FALSE;
    break;

    case 'sem':
      $sems = array_flip($vals['semester']);
      unset($sems[0]);
      if (!$sems) {
        $val = FALSE;
        form_set_error('semsester', 'Choose at least 1 semester');
      }
    break;

    case 'email':
      //get rid of valid tokens
      $patterns = array('{firstname}', '{lastname}', '{nickname}');
      $subject = str_replace($patterns, '', $vals['subject']);
      $details = str_replace($patterns, '', $vals['details']);

      //now search for tokens: any found are invalid
      for ($field='subject'; $field; $field=($field=='subject'?'details':'')) {
        if (strpos($$field, '{') !== FALSE || strpos($$field, '}') !== FALSE) {
          form_set_error($field, 'The message '.($field=='subject'?$field.' ':'').'contains an invalid token. Correct tokens are {firstname}, {lastname}, and {nickname}');
          $val = FALSE;
        }
      }
  }
  return $val;
}

function ws_admissions_prospective_form_submit($form, $form_state) {
  $vals = $form_state['values'];

  $c = 0;
  foreach ($vals['cases'] as $case_contact => $checked) {
    if ($checked) {
      list($case_id, $contact_id) = explode('-', $case_contact);
      $cases[$case_id] = $contact_id;
      $acts[$case_id] = 0;
      ++$c;
    }
  }
  $date = woolman_date_array_to_string($vals['date']) . $vals['time'];

  switch($vals['actions']) {

    case 'status':
      $params = array('status_id' => $vals['change_status']);
      foreach ($cases as $params['id'] => $contact_id) {
        $result = woolman_civicrm_api('case', 'create', $params);
      }
      $db = db_query('SELECT label FROM {civicrm_option_value} WHERE option_group_id = 27 AND value = %d', $vals['change_status']);
      $status = db_fetch_array($db);
      ws_admissions_record_activity($acts, 'Status Changed to ' . $status['label'], $vals['subject'], 1, date('YmdHis'), 16, 2, TRUE);
      break;

    case 'rep':
      db_query('UPDATE {civicrm_relationship} SET contact_id_b = %d
        WHERE relationship_type_id = 13 AND case_id IN ('.implode(',',array_keys($cases)).')', $vals['change_rep']);
      break;

    case 'grad':
      $params = array(
        'contact_type' => 'Individual',
        'custom_4' => $vals['grad'] . '0601',
      );
      foreach ($cases as $params['contact_id']) {
        woolman_civicrm_api('contact', 'create', $params);
      }
      break;

    case 'sem':
      $db = db_query('SELECT case_id, activity_id
        FROM {civicrm_case_activity} ca
        WHERE activity_id IN (SELECT id FROM {civicrm_activity} WHERE activity_type_id = 47)
        AND case_id IN ('. implode(',', array_keys($cases)) .')'
      );
      while ($row = db_fetch_array($db)) {
        $acts[$row['case_id']] = $row['activity_id'];
      }
      $sems = array_flip($vals['semester']);
      unset($sems[0]);
      $choices = array();
      foreach ($sems as $sem) {
        $choices[] = $form['params']['semester']['#options'][$sem];
      }
      $subject = implode(' or ', $choices);

      $params = array(
        'activity_type_id' => 47,
        'source_contact_id' => woolman_user_cid(),
        'status_id' => 2,
        'medium_id' => 1,
        'subject' => $subject,
        'custom_5' => woolman_multivalue_str($sems),
      );
      foreach ($acts as $params['case_id'] => $params['id']) {
        woolman_civicrm_api('activity', 'create', $params);
      }
      break;

    case 'followup':
      ws_admissions_record_activity($acts, 'Follow Up', '', 2, substr($date, 0, 8), 14, 1, TRUE);
      break;

    case 'left-msg':
      ws_admissions_record_activity($acts, 'Left Msg', $vals['subject'], 2, $date, 14, 3, TRUE);
      break;

    case 'act-14': case 'act-28': case 'act-29':
      $type = substr($vals['actions'], -2);
      ws_admissions_record_activity($acts, $vals['subject'], $vals['details'], $vals['medium'], $date, $type);
    break;

    case 'act-31':
      //check for existing, since only one of this act type is allowed
      $db = db_query('SELECT DISTINCT ca.case_id, con.display_name
                      FROM {civicrm_case_activity} ca
                      JOIN {civicrm_case_contact} cc ON ca.case_id = cc.case_id
                      JOIN {civicrm_contact} con ON cc.contact_id = con.id
                      WHERE ca.activity_id IN (SELECT id FROM {civicrm_activity} WHERE activity_type_id = 31 AND is_current_revision = 1 AND is_deleted = 0 AND status_id <> 1)
                      AND ca.case_id IN ('. implode(',', array_keys($acts)) .')');
      while ($row = db_fetch_array($db)) {
        drupal_set_message('Didn\'t create Facebook activity for ' . $row['display_name'].' because it already exists.', 'warning');
        unset($acts[$row['case_id']]);
        --$c;
      }
      if ($acts) {
        ws_admissions_record_activity($acts, $vals['subject'], '',  6, $date, 31);
      }
    break;

    case 'email':
      $self = woolman_contact_get();
      $contact_case = array_flip($cases);
      $students = array();
      $db = db_query('SELECT con.id, con.first_name, con.nick_name, con.last_name, em.email
                      FROM {civicrm_contact} con
                      LEFT JOIN {civicrm_email} em ON em.contact_id = con.id AND em.is_primary = 1
                      WHERE con.id IN ('.implode(',', $cases).')
                      ORDER BY con.sort_name');

      while ($row = db_fetch_array($db)) {
        if ($row['email']) {
          $students[] = array(
            'id' => $row['id'],
            'first_name' => $row['first_name'],
            'last_name' => $row['last_name'],
            'nick_name' => ($row['nick_name'] ? $row['nick_name'] : $row['first_name']),
            'email' => $row['email'],
            'case' => $contact_case[$row['id']],
          );
        }
        else {
          drupal_set_message("Couldn't send message to {$row['first_name']} {$row['last_name']}: no email address.", 'warning');
        }
      }
      $patterns = array('/{firstname}/i', '/{lastname}/i', '/{nickname}/i');
      $lang = language_default();
      $c = 0;
      $from = '"' . $self['nick_name'] . ' ' . $self['last_name'] . '" <' . $self['email'] . '>';
      $date = date('YmdHis');

      foreach ($students as $s) {
        $message = array();
        $replacements = array($s['first_name'], $s['last_name'], $s['nick_name'],);

        $to = '"' . $s['first_name'].' ' . $s['last_name'].'" <' . $s['email'].'>';
        $subject = $message['subject'] = preg_replace($patterns, $replacements, $vals['subject']);
        $details = $message['body'] = preg_replace($patterns, $replacements, $vals['details']);

        $mail_result = drupal_mail('woolman_website', 0, $to, $lang, $message, $from);

        if ($mail_result['result'] != FALSE) {
          ++$c;
          $success[] = $s['first_name'].' ' . $s['last_name'];
          ws_admissions_record_activity(array($s['case']=> 0), $subject, $details, 3, $date, 14);
        }
        else // unable to send mail
          $failure[] = $s['first_name'].' ' . $s['last_name'];
      }
      if (!empty($success))
        drupal_set_message('Sent email to ' . $c.' student'.($c==1?'':'s').': '.implode(', ', $success));
      if (!empty($failure))
        drupal_set_message('Sorry, an error occurred while trying to email '.($c==1?'':'the following students: ').implode(', ', $failure), 'error');

    break;
  }
  drupal_set_message('Updated ' . $c.' case'.($c==1?'':'s').'.');
  $qs = $_GET;
  unset($qs['q']);
  unset($qs['emailprompt']);
  drupal_goto($_GET['q'], $qs);
}

/**
 * Form to control report
 */
function ws_admissions_prospective_controls($form_state, $reps, $stats, $states, $trip) {
  if (!empty($_GET['status'])) {
    $default_stats = explode('-', $_GET['status']);
  }
  else {
    $default_stats = array();
  }
  $times = array(
    '- Any Time -',
    'lt-1-week' => 'Less than a week ago',
    'lt-2-week' => 'Less than 2 weeks ago',
    'lt-1-month' => 'Less than a month ago',
    'lt-3-month' => 'Less than 3 months ago',
    'lt-6-month' => 'Less than 6 months ago',
    'lt-1-year' => 'Less than a year ago',
    'lt-2-year' => 'Less than 2 years ago',
    'lt-3-year' => 'Less than 3 years ago',
    'gt-1-week' => 'More than a week ago',
    'gt-2-week' => 'More than 2 weeks ago',
    'gt-1-month' => 'More than a month ago',
    'gt-3-month' => 'More than 3 months ago',
    'gt-6-month' => 'More than 6 months ago',
    'gt-1-year' => 'More than a year ago',
    'gt-2-year' => 'More than 2 years ago',
  );

  if ($trip) {
    $trip = $trip['subject'].' (ID#' . $trip['id'].')';
  }
  if (!empty($_GET['school']) && ($hs = woolman_contact_get($_GET['school']))) {
    $school = $hs['display_name'].' (ID#' . $_GET['school'].')';
  }

  foreach ($stats as $st => &$sta) {
    $sta = '<span></span>' . $sta;
  }

  return array(
    'status' => array(
      '#type' => 'checkboxes',
      '#options' => $stats,
      '#default_value' => $default_stats,
    ),
    'rep' => array(
      '#type' => 'select',
      '#title' => 'Admissions Rep',
      '#options' => $reps,
      '#default_value' => empty($_GET['rep']) ? 0 : $_GET['rep'],
      '#prefix' => '',
    ),
    'start' => array(
       '#type' => 'select',
      '#title' => 'Start Date',
      '#options' => $times,
      '#default_value' => empty($_GET['start']) ? 0 : $_GET['start'],
    ),
    'act' => array(
      '#type' => 'select',
      '#title' => 'Latest Completed Activity',
      '#options' => $times,
      '#default_value' => empty($_GET['act']) ? 0 : $_GET['act'],
    ),
    'pending' => array(
      '#type' => 'select',
      '#title' => 'Next Pending Activity',
      '#options' => array(
        '- Any -',
        'overdue' => 'Overdue',
        'lt-1-week' => 'Due in less than 1 week',
        'lt-2-week' => 'Due in less than 2 weeks',
        'lt-3-week' => 'Due in less than 3 weeks',
        'lt-1-month' => 'Due in less than 1 month',
        'lt-2-month' => 'Due in less than 2 months',
        'gt-1-week' => 'Not due for over 1 week',
        'gt-2-week' => 'Not due for over 2 weeks',
        'gt-3-week' => 'Not due for over 3 weeks',
        'gt-1-month' => 'Not due for over 1 month',
        'none' => 'No pending activities',
      ),
      '#default_value' => empty($_GET['pending']) ? 0 : $_GET['pending'],
    ),
    'state' => array(
      '#type' => 'select',
      '#title' => 'State',
      '#options' => $states,
      '#default_value' => empty($_GET['state']) ? 0 : $_GET['state'],
    ),
    'trip' => array(
      '#type' => 'textfield',
      '#size' => 20,
      '#title' => 'Outreach Trip',
      '#default_value' => $trip,
      '#autocomplete_path' => 'staff/js/autocomplete/trip',
    ),
    'school' => array(
      '#type' => 'textfield',
      '#size' => 20,
      '#title' => 'High School',
      '#default_value' => $school,
      '#autocomplete_path' => 'staff/js/autocomplete/hs',
    ),
    'name' => array(
      '#type' => 'textfield',
      '#size' => 20,
      '#title' => 'Student',
      '#default_value' => empty($_GET['name']) ? '' : $_GET['name'],
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => 'View Students',
      '#prefix' => '<div id="submit-wrapper" class="form-item"><label>&nbsp;</label>',
      '#suffix' => '</div>',
    ),
  );
}

/**
 * Submit callback for report controls
 */
function ws_admissions_prospective_controls_submit($form, $form_state) {
  module_load_include('inc', 'woolman_semester', 'ws_admissions_utils');
  $vals = $form_state['values'];
  $q = array('status' => '');

  foreach ($form_state['values']['status'] as $stat => $selected) {
    if ($selected) {
      $q['status'] .= ($q['status'] ? '-' : '') . $stat;
    }
  }
  $q['status'] = $q['status'] ? $q['status'] : '1-3';

  foreach (array('rep', 'start', 'act', 'pending', 'state', 'name', 'trip', 'school') as $item) {
    if (!empty($vals[$item]) && ($item == 'trip' || $item == 'school')) {
      $thing = woolman_split_name_id($vals[$item]);
      if (!empty($thing['id']) && is_numeric($thing['id'])) {
        $q[$item] = $thing['id'];
      }
    }
    elseif (!empty($vals[$item])) {
      $q[$item] = $vals[$item];
    }
  }

  drupal_goto('staff/admissions/prospectives', $q) ;
}

/**
 * Make date look pretty
 * @param array/str: activity array fetched from civicrm activity api OR timestamp
 * @param bool: whether to return the time as well as the date
 * @return Month, Date (time, if desired) (year, if not this year)
 */
function _ws_admissions_date_format($act, $format='short') {
  if (is_array($act))
    $date = strtotime($act['activity_date_time']);
  else
    $date = strtotime($act);

  switch($format) {
  case 'long':
    $date_display = date('F jS',$date);
    if (date('FjY',$date) == date('FjY'))
        $date_display = 'today';

    if (date('Y', $date) != date('Y'))
      $date_display .= ', '.date('Y', $date);
  break;
  case 'short':
    $date_display = date('n/j/y',$date);
  }
  return $date_display;
}

/**
 * Batch create/update case activities
 */
function ws_admissions_record_activity($acts, $subject, $details, $medium, $date, $type, $status=2, $create_only=FALSE) {
  if (!$create_only) {
    $db = db_query(
      'SELECT case_id, activity_id
        FROM {civicrm_case_activity}
        WHERE activity_id IN
          (SELECT id FROM {civicrm_activity} WHERE activity_type_id = ' . $type . '
          AND is_current_revision = 1 AND is_deleted = 0 AND status_id = 1)
        AND case_id IN (' . implode(',', array_keys($acts)) . ')
        ORDER BY activity_id DESC'
    );
    while ($row = db_fetch_array($db)) {
      $acts[$row['case_id']] = $row['activity_id'];
    }
  }
  $params = array(
    'activity_type_id' => $type,
    'source_contact_id' => woolman_user_cid(),
    'status_id' => $status,
    'activity_date_time' => $date,
    'medium_id' => $medium,
  );
  if ($subject) {
    $params['subject'] = $subject;
  }
  if ($details) {
    $params['details'] = '<p>' . nl2br($details) . '</p>';
  }

  foreach ($acts as $params['case_id'] => $params['id']) {
    woolman_civicrm_api('activity', 'create', $params);
  }
}
