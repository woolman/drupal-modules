<?php

module_load_include('inc', 'woolman_camp', 'woolman_camp_utils');

/**
 * Main page for camp management
 */
function woolman_camp_staff() {
  $path = drupal_get_path('module', 'woolman_camp');
  drupal_add_css($path . '/css/camp_staff.css', 'module', 'all', FALSE);
  drupal_add_js('sites/all/libraries/tablesorter/jquery.tablesorter.min.js');
  drupal_add_js($path . '/js/staff.js');
  popups_add_popups();
  // Status options for in-place editing
  $setting = array(
    'status' => array(
      1 => 'Registered',
      2 => 'Attended',
      7 => 'On waitlist',
      4 => 'Cancelled',
      3 => 'No-show',
      13 => 'Left Early',
      11 => 'Rejected',
    ),
  );
  drupal_add_js(array('camp_staff' => $setting), 'setting');

  $next_year = $year = woolman_camp_next_year();
  $years = drupal_map_assoc(range(2005, $year));
  if (!empty($_REQUEST['year']) && array_key_exists($_REQUEST['year'], $years)) {
    $year = $_REQUEST['year'];
  }
  $data = $camper_sort = $stats = $reg = array();
  $programs = woolman_camp_event_types();
  $sessions = woolman_camp_get_sessions($year, TRUE);
  foreach ($sessions as $id => $session) {
    $data[$id] = array_keys($session['campers']);
  }
  $cache = cache_get("woolman_camp_staff_session_$year");
  $timestamp = $cache->created;
  $cache = $cache->data;

  // Get parents & forms
  $query = db_query("
    SELECT p.id, p.contact_id AS camper, con.first_name, con.last_name, con.nick_name, n.parent_15
    FROM {civicrm_participant} p, {civicrm_value_registration_questions_7} n, {civicrm_contact} con
    WHERE n.entity_id = p.id AND n.parent_15 = con.id AND p.event_id IN (".implode(',',array_keys($sessions)).")
    GROUP BY p.contact_id");
  while ($c = db_fetch_array($query)) {
    $parents[$c['camper']] = '<a target="_blank" href="/civicrm/contact/view?reset=1&cid=' . $c['parent_15'] . '">' . $c['first_name'] . '<i></i>' . ($c['nick_name'] ? ' (' . $c['nick_name'] . ')' : '') . ' <i></i>' . $c['last_name'] .'</a>';
    $reg[$c['camper']] = $c['id'];
    $parent_id[$c['camper']] = $c['parent_15'];
    $fees[$c['camper']] = l('+', 'staff/camp/viewform/add-payment/' . $c['parent_15'] . '/' . $year, array('alias' => TRUE, 'attributes' => array('class' => 'popups-form a-p', 'title' => 'New Payment')));
    $campership[$c['camper']] = l('-', 'staff/camp/viewform/add-discount/' . $c['parent_15'] . '/' . $year, array('alias' => TRUE, 'attributes' => array('class' => 'popups-form a-p', 'title' => 'Add discount/campership')));
  }

  foreach ($sessions as $id => $session) {
    if (!isset($cache[$id]) || $data[$id] !== $cache[$id]) {
      woolman_camp_staff_graph_session($session);
    }
    if (empty($camper_sort[$session['event_type_id']])) {
      $camper_sort[$session['event_type_id']] = array();
    }
    $registered = $waitlisted = $boys = $girls = 0;
    foreach ($session['campers'] as $c) {
      if ($c['status'] == 'On waitlist') {
        $waitlisted++;
      }
      elseif ($c['class'] != 'Negative') {
        $registered++;
        if ($c['gender_id'] == 1) {
          ++$girls;
        }
        elseif ($c['gender_id'] == 2) {
          ++$boys;
        }
      }
      if ($c['status'] == 'Awaiting approval') {
        $c['class'] = 'needs-approval';
      }
      if (empty($campers[$c['contact_id']])) {
        $campers[$c['contact_id']] = $c;
        $campers[$c['contact_id']]['status'] = array();
      }
      else {
        $campers[$c['contact_id']]['fee_amount'] += $c['fee_amount'];
      }
      $span = '<span ' . ($c['status_id'] == 8 ? 'class="' : 'title="Change status" class="p-s ') . strtolower($c['class']) . '" data-sid="' . $c['status_id'] . '">' . $c['status'];
      $campers[$c['contact_id']]['status'][$span][$c['participant_id']] = $campers[$c['contact_id']]['sessions'][$id] = trim(str_replace("$year", '', $session['title']));
      $camper_sort[$session['event_type_id']][$c['last_name'] . $c['nick_name'] . $c['first_name'] . $c['contact_id']] = $c['contact_id'];
    }
    $stats[$session['event_type_id']] .=
      '<div class="session">
        <h4>' . $session['title'] . '</h4>
        <div class="session-inner">
          <div>' . woolman_camp_date_format($session['start_date'], $session['end_date']) . '</div>
          <div><strong>Limit: </strong>' . $session['max_participants'] . '</div>
          <div><strong>Registered*: </strong>' . $registered.' (' . $girls . ' girl' . ($girls == 1 ? '' : 's') . ' &amp; ' . $boys . ' boy' . ($boys == 1 ? '' : 's') . ')</div>
          <div><strong>Waitlisted: </strong>' . $waitlisted.'</div>
          <div class="age-graph"><img src="/sites/default/files/woolman_graphs/session' . $session['id'] . '.png?ts=' . $timestamp . '" /></div>
        </div>
      </div>';
  }

  if ($cache != $data) {
    cache_set('woolman_camp_staff_session_' . $year, $data, 'cache', strtotime('+1 day'));
    $cache = cache_get('woolman_camp_staff_session_' . $year);
    $timestamp = $cache->created;
    $cache = $cache->data;
    woolman_camp_staff_graph_registration($year);
  }

  $output = '<div id="camp-staff">';
  // We started tracking reg-dates in 2011
  if ($year > 2011) {
    $output .= '<div id="registration-graph"><img src="/sites/default/files/woolman_graphs/year-' . $year . '.png?ts=' . $timestamp . '"></div>';
  }

  $thead = array(
    array('data' => 'Name', 'class' => 'header headerSortDown'),
    array('data' => 'Age*'),
    array('data' => 'Sex'),
    array('data' => 'From'),
    array('data' => 'Since'),
    array('data' => 'Registered by'),
    array('data' => 'Sessions'),
    array('data' => 'Status'),
    array('data' => 'Forms'),
    array('data' => 'Paid'),
  );
  foreach ($camper_sort as $program => $kids) {
    ksort($kids);
    $tbody = array();
    foreach ($kids as $cid) {
      $c = $campers[$cid];

      $forms = '';

      $rowclass = '';
      if (count($c['status']) > 1) {
        $status = '';
        foreach ($c['status'] as $s => $t) {
          if ($status) {
            $status .= ', ';
          }
          $status .= str_replace('data-sid', 'data-pid="' . implode(',', array_keys($c['status'][$s])) . '" data-sid', $s) . ' (' . implode(', ', $t) . ')</span>';
        }
      }
      else {
        $s = array_keys($c['status']);
        $status = str_replace('data-sid', 'data-pid="' . implode(',', array_keys($c['status'][$s[0]])) . '" data-sid', $s[0]) . '</span>';
        if ($c['class'] == 'Negative') {
          $rowclass = 'negative';
        }
      }
      if ($needs_approval = strpos($status, 'approval')) {
        $approval_link = l("Needs Approval", 'staff/camp/viewform/review/' . $reg[$cid], array('alias' => TRUE, 'attributes' => array('class' => 'popups-form needs-approval', 'title' => 'Review Application')));
        $status = str_replace('Awaiting approval', $approval_link, $status);
      }
      if (!empty($reg[$cid])) {
        $forms .= l('App', 'staff/camp/viewform/' . ($needs_approval ? 'review' : 'app') . '/' . $reg[$cid], array('alias' => TRUE, 'attributes' => array('class' => 'popups', 'title' => ($needs_approval ? 'Rev' : 'V') . 'iew Application')));
      }
      if (!empty($c['medform_date'])) {
        $forms .= ' ' . l('Med', 'civicrm/contact/view', array('query' => 'reset=1&cid=' . $cid . '&selectedChild=custom_9', 'alias' => TRUE, 'attributes' => array('target' => '_blank', 'title' => 'View Medical Form')));
      }
      $paid = '';
      if ($c['fee_amount'] || $year>=2011) {
        $paid = '$' . number_format($c['fee_amount'], 2);
        if ($parent_id[$cid])
          $paid = l($paid, 'staff/camp/viewform/payments/' . $parent_id[$cid] . '/' . $year, array('alias' => TRUE, 'attributes' => array('class' => 'popups')));
      }

      $tbody[] = array('class' => $rowclass, 'data' => array(
        '<a target="_blank" href="/civicrm/contact/view?reset=1&cid=' . $c['contact_id'] . '">' . $c['first_name'] .'<i></i>'. ($c['nick_name'] ? ' (' . $c['nick_name'] . ')' : '') . ($c['middle_name'] ? ' ' . $c['middle_name'][0] . '.' : '') . ' <i></i>' . $c['last_name'] . '<i></i></a>',
        $c['age'],
        array('data' => ($c['gender_id'] == 1 ? 'F' : 'M'), 'class' => 'gender-' . $c['gender_id']),
        $c['city'],
        ($c['camper_since']==$year?'':$c['camper_since']),
        $parents[$cid],
        implode(', ',$c['sessions']),
        $status,
        $forms,
        $paid . ' ' . $fees[$cid] . $campership[$cid],
      ));
    }
    $table = theme('table', $thead, $tbody, array('class' => 'tablesorter'));
    $output .= '<div class="camp-session">
                  <h2 class="underline">' . $programs[$program]['name'] . '</h2>
                  <div class="session-stats">' . $stats[$program] . '
                    <p style="font-size:0.8em;clear:both;">*Campers who have not yet been approved are included in these numbers (be sure to approve them ASAP!) Campers who are waitlisted, cancelled, or rejected are not included in the graphs, only in the table below.</p>
                  </div>
                  <div>' . $table . '<div style="font-size:0.8em;clear:both;">*All ages are as of July 1, ' . $year.'</div></div>
                </div>';
  }

  // Shuttles
  $output .= '<div class="shuttles-outer">';
  foreach (woolman_camp_get_shuttle($year, TRUE) as $dir => $sh) {
    $output .= '<div class="shuttles ' . $dir . '">
                  <h4>' . ($dir == 'coming' ? 'Shuttles to Camp (Session Start)' : 'Shuttles from Camp (Session End)') . '</h4>';
    foreach ($sh as $s) {
      $c = count($s['campers']);
      $output .= '<div class="shuttle' . ($c ? '' : ' empty') . '">
                    <h5>' . woolman_camp_date_format($s['start_date']) . ': (' . $c . ' rider' . ($c == 1 ? '' : 's') . ')</h5>';
      if ($c) {
        $campers = array();
        foreach ($s['campers'] as $c) {
          $campers[$c['sort_name']] = '<li>' . $c['display_name'] . '</li>';
        }
        ksort($campers);
        $output .= '<ul>' . implode("\n", $campers) . '</ul>';
      }
      $output .= '</div>';
    }
    $output .= '</div>';
  }
  $output .= '</div></div>';

  return drupal_get_form(woolman_camp_years_nav, $years, $year) . $output;
}

/**
 * Navigation form for main camp staff page
 */
function woolman_camp_years_nav(&$form_state, $years, $year) {
  return array(
    'year_select' => array(
      '#type' => 'select',
      '#attributes' => array('onchange' => '$(\'#woolman-camp-years-nav\')[0].submit();'),
      '#options' => $years,
      '#title' => 'Viewing Summer of',
      '#default_value' => $year,
      '#prefix' => '<div class="subset">',
      ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => 'Go'
    ),
    'report' => array(
      '#prefix' => '<div class="form-item">',
      '#value' => l("View $year Report", 'staff/camp/viewform/fee-report/0/' . $year, array('alias' => TRUE, 'attributes' => array('class' => 'popups', 'title' => 'Enrollment and Payment Data for ' . $year, 'on-popups-options' => '{width: "90%"}'))),
      '#suffix' => '</div>',
    ),
    'discounts' => array(
      '#prefix' => '<div class="form-item">',
      '#value' => l('Manage Discounts', 'staff/camp/viewform/discounts/0/' . $year, array('alias' => TRUE, 'attributes' => array('class' => 'popups', 'title' => 'Add or edit discounts'))),
      '#suffix' => '</div>',
    ),
    'age' => array(
      '#prefix' => '<div class="form-item">',
      '#value' => l('Add Age Adjustment', 'staff/camp/viewform/age/0/' . $year, array('alias' => TRUE, 'attributes' => array('class' => 'popups', 'title' => 'Grant an age exception to a camper'))),
      '#suffix' => '</div></div>',
    ),
  );
}

/**
 * Submit handler for camp navigation form
 */
function woolman_camp_years_nav_submit($form, &$form_state) {
  drupal_goto('staff/camp/registration', array('year' => $form_state['values']['year_select']));
}

/**
 * AJAX page callback to load a back-end form
 */
function woolman_camp_viewform() {
  $op = arg(3);
  $id = arg(4);
  $year = arg(5);
  if (!is_numeric($id)) {
    return drupal_not_found();
  }
  $output = 'Sorry, couldn\'t load the requested information.';
  switch($op) {
    case 'app':
      if ($reg = woolman_camp_get_app($id)) {
        $output = $reg[1];
      }
    break;
    case 'review':
      if ($reg = woolman_camp_get_app($id)) {
        list($data, $app) = $reg;
        $output = $app.drupal_get_form('woolman_camp_approval_form', $data);
      }
    break;
    case 'payments':
      $output = woolman_camp_payment_summary($id, $year);
    break;
    case 'age':
      $output = drupal_get_form('woolman_camp_age_adjust', $id, $year);
    break;
    case 'add-payment':
      drupal_set_title('Record A New Payment');
      if ($reg = woolman_camp_calc_fees($id, $year)) {
        $output = drupal_get_form('woolman_camp_add_payment', $id, $reg, $year);
      }
    break;
    case 'add-discount':
      drupal_set_title('Add Discount');
      if ($reg = woolman_camp_calc_fees($id, $year)) {
        $output = drupal_get_form('woolman_camp_add_discount', $id, $reg, $year);
      }
    break;
    case 'discounts':
      $output = drupal_get_form('woolman_camp_manage_discount');
    break;
    case 'fee-report':
      drupal_set_title($year.' Camp Fee Report');
      $output = woolman_camp_fee_report($year);
    break;
  }
  return '<div id="staff-form" style="min-height:300px;">' . $output . '</div>';
}

/**
 * Display an application form
 */
function woolman_camp_get_app($id) {
  $query = db_query('
    SELECT p.contact_id as cid, n.parent_15 AS pid, n.*, p.register_date
    FROM {civicrm_participant} p, {civicrm_value_registration_questions_7} n
    WHERE n.entity_id = p.id AND p.id = %d', $id);
  if ($data = db_fetch_array($query)) {
    $data['camper_first'] = woolman_name('nick', $data['cid']);
    $data['camper_name'] = woolman_name('full', $data['cid']);
    $data['parent_name'] = woolman_name('full', $data['pid']);
    $date = date('M j Y', strtotime($data['register_date']));
    drupal_set_title('Application for ' . $data['camper_name']);
    $text = '<em>Submitted by ' . $data['parent_name'] . ' on ' . $date.'</em>
      <h4>Camper Questions</h4>'
      .nl2br(html_entity_decode($data['camper_question_1_10']))
      .nl2br(html_entity_decode($data['camper_question_2_12']))
      .'<h4>Parent Questions</h4>'
      .nl2br(html_entity_decode($data['parent_question_1_13']))
      .nl2br(html_entity_decode($data['parent_question_2_14']));
    return array($data, $text);
  }
}

/**
 * Display a table of fees and payments
 */
function woolman_camp_payment_summary($id, $year) {
  $programs = woolman_camp_event_types();
  $reg = woolman_camp_calc_fees($id, $year);
  $dbody = $pbody = $cbody = array();
  drupal_set_title('Fees and Payments for ' . woolman_name('display', $id));

  $dhead = array('Item', 'Date', 'Amount');
  foreach ($reg['sessions'] as $type => $sessions) {
    if ($s = $sessions['count']) {
      $dbody[] = array(
        $s . ' Session' . ($s == 1 ? '' : 's') . ' of ' . $programs[$type]['name'],
        date('M j, Y', strtotime($reg['register_date'])),
        '$' . $sessions['price'],
      );
    }
  }
  if ($w = $reg['shuttles']) {
    $dbody[] = array(
      $w . ' Shuttle Ride' . ($w == 1 ? '' : 's'),
      '',
      '$' . $reg['shuttle_price'],
    );
  }
  foreach ($reg['discounts'] as $title => $info) {
    $dbody[] = array(
      'data' => array(
        $title,
        date('M j, Y', strtotime($info['date'])),
        '-$' . $info['amount'],
      ),
      'style' => 'font-style:italic;',
    );
  }
  $dbody[] = array(
    'data' => array(
      'Fees minus discounts',
      '',
      '$' . ($reg['total_fees'] - $reg['total_discount'])
    ),
    'style' => 'font-weight:bold;',
  );

  $phead = array('Paid By', 'Date', 'Method', 'Amount');
  foreach ($reg['payments'] as $count => $payment) {
    $pbody[] =  array(
      $payment['display_name'],
      date('M j, Y', strtotime($payment['receive_date'])),
      $payment['method'],
      '$' . $payment['total_amount'],
    );
  }
  $pbody[] = array(
    'data' => array('Total Paid', '', '', '$' . $reg['total_paid']),
    'style' => 'font-weight:bold;',
  );
  $pbody[] = array(
    'data' => array(
      'Outstanding Balance',
      '',
      '',
      '$' . ($reg['total_fees']-$reg['total_discount']-$reg['total_paid']),
    ),
   'style' => 'font-weight:bold;font-style:italic;'
  );
  $chead = array('Name', 'Sessions', 'Total Paid');
  foreach ($reg['campers'] as $camper) {
    $cbody[] = array(
      $camper['first_name'],
      $camper['session_names'],
      '$' . $camper['total_paid'],
    );
  }
  return theme('table', $dhead, $dbody, array(), 'Fees and Discounts') .
    theme('table', $phead, $pbody, array(), 'To-Date Payments for Camp ' . $year) .
    theme('table', $chead, $cbody, array(), 'Campers');
}

/**
 * Form builder for staff to approve/reject a camp application
 */
function woolman_camp_approval_form(&$form_state, $data) {
  $form_state['storage']['data'] = $data;
  return array(
    '#prefix' => '<hr /><p style="font-size:0.8em;">Click accept to approve the registration. A friendly email will automatically be sent to the parent with instructions. Click deny to reject the application. No automatic message will be sent (you\'ll have to break the news yourself).</p>',
    '#redirect' => array('staff/camp/registration', "year=$year"),
    'accept' => array(
      '#type' => 'submit',
      '#value' => 'Accept Camper'
    ),
    'reject' => array(
      '#type' => 'submit',
      '#value' => 'Deny Application',
      '#suffix' => '<a class="cancel-link" href="/staff/camp/registration" onclick="$(\'.popups-close\').click(); return false;">Cancel (do nothing)</a>'
    ),
  );
}

/**
 * Submit handler for staff to approve/reject a camp application
 */
function woolman_camp_approval_form_submit($form, &$form_state) {
  $data = $form_state['storage']['data'];
  unset($form_state['storage']);
  $parent = woolman_contact_get($data['pid']);
  $event_types = array_keys(woolman_camp_event_types());

  switch($form_state['values']['op']) {
    case 'Accept Camper':
      $status = 1;
      $to = '"' . $parent['display_name'] . '" <' . $parent['email'] . '>';
      $from = '"Woolman Camps" <camp@woolman.org>';
      $message['subject'] = 'Your camp application has been accepted';
      $message['body'] = "Hello {$parent['nick_name']}\n\nWe've got good news for you! Your camper application for {$data['camper_first']} has been accepted. You are now ready to finalize your registration online by completing med forms and paying your deposit and fees. You'll find everything you need at:\nhttp:// Camp.woolman.org/register/status\n\nRemember, {$data['camper_first']}'s place at camp is not reserved until your deposit is received. You can find out details about making this payment by clicking the above link.\n\nWe're so excited to see {$data['camper_first']} at camp this summer!\n\nSincerely,\nYour Friendly Woolman Camp Staff\n530-273-3183x3\nhttp:// Camp.woolman.org";
      $message['to_cid'] = $parent['contact_id'];
      drupal_mail('woolman_website', 'create_activity', $to, language_default(), $message, $from);
      drupal_set_message($data['camper_name'] . '\'s application has been accepted. A friendly email has been sent to ' . $parent['display_name'] . ' with further instructions.');
    break;
    case 'Deny Application':
      $status = 11;
      drupal_set_message($data['camper_name'] . '\'s application has been denied. An automatic email has <strong>not</strong> been sent to ' . $parent['display_name'] . '; you\'ll have to break the news yourself.');
    break;
  }
  db_query('UPDATE {civicrm_participant} SET status_id = ' . $status . '
            WHERE contact_id = %d AND status_id = 8 AND event_id IN
            (SELECT id FROM {civicrm_event} WHERE event_type_id IN (' . implode(',', $event_types) . '))', $data['cid']);
}

/**
 * Form builder for staff to adjust camper age
 */
function woolman_camp_age_adjust($form_state, $cid, $year) {
  $form = array(
    '#prefix' => '<p><em>This allows campers to register for a session that they are otherwise too old or too young for.</em></p>',
    '#redirect' => array('staff/camp/registration', "year=$year"),
  );
  if (empty($form_state['storage']['cid']) && empty($cid)) {
    drupal_set_title('Age Adjustment');
    $form['camper'] = array(
      '#type' => 'textfield',
      '#size' => 50,
      '#title' => 'Camper Name',
      '#description' => '<em>Note: This person needs to exist in the database before you can grant an age exception - ask the parent to fill out the basic "family info" form if they haven\'t already.</em>',
      '#required' => TRUE,
      '#autocomplete_path' => 'staff/js/autocomplete/Individual',
      '#suffix' => '<p style="height:200px;">&nbsp;</p>',
    );
    $form['submit-camper'] = array(
      '#type' => 'submit',
      '#value' => 'Continue',
    );
    $form_state['storage']['year'] = $year;
  }
  else {
    $cid = woolman_aval($form_state, 'storage:cid', $cid);
    $camper = woolman_contact_get($cid);
    $real_age = woolman_camp_age($camper['birth_date'], $year);
    drupal_set_title('Age Adjustment for ' . $camper['display_name']);
    $form['#prefix'] .= '
      <table><tbody>
        <tr><td>Birth Date:</td><td>' . date('M j Y', strtotime($camper['birth_date'])) . '</td></tr>
        <tr><td>Real Age (as of July 1 ' . $year . '):</td><td>' . $real_age . '</td></tr>
        <tr><td>Adjusted Age:</td>';
    $form['age'] = array(
      '#type' => 'select',
      '#options' => drupal_map_assoc(range($real_age - 2, $real_age + 2)),
      '#default_value' => woolman_camp_age($camper['birth_date'], $year, $cid),
      '#prefix' => '<td>',
      '#suffix' => '</td></tr>',
    );
    $form['closure'] = array('#value' => '</tbody></table>');
    $form['submit-age'] = array(
      '#type' => 'submit',
      '#value' => 'Save Age',
    );
  }
  return $form;
}

/**
 * Submit handler for staff to adjust camper age
 */
function woolman_camp_age_adjust_submit($form, &$form_state) {
  if (!($cid = woolman_aval($form_state, 'storage:cid'))) {
    $camper = woolman_split_name_id($form_state['values']['camper']);
    $form_state['storage']['cid'] = $camper['id'];
    $form_state['rebuild'] = TRUE;
    return;
  }
  else {
    $year = $form_state['storage']['year'];
    unset($form_state['storage']);
    $camper = woolman_contact_get($cid);
    $real_age = woolman_camp_age($camper['birth_date'], $year);
    $delta = $form_state['values']['age'] - $real_age;
    if ($delta === 0) {
      db_query("DELETE FROM {woolman_camp_age_exception} WHERE year = %d AND cid = %d", $year, $cid);
      return;
    }
    $query = db_query("SELECT delta FROM {woolman_camp_age_exception} WHERE year = %d AND cid = %d", $year, $cid);
    if ($exception = db_fetch_object($query)) {
      db_query("UPDATE {woolman_camp_age_exception} SET delta = %d WHERE year = %d AND cid = %d", $delta, $year, $cid);
    }
    else {
      db_query("INSERT INTO {woolman_camp_age_exception} (delta, year, cid) VALUES (%d, %d, %d)", $delta, $year, $cid);
    }
  }
}

/**
 * Form builder for staff to enter camp payment
 */
function woolman_camp_add_payment(&$form_state, $cid, $reg, $year) {
  $form_state['storage']['reg'] = $reg;
  $form_state['storage']['year'] = $year;
  $form_state['storage']['parent'] = $parent = woolman_contact_get($cid);

  $form = array(
    '#prefix' => '<p>Add payment for ' . $parent['display_name'] . '\'s family</p>',
    '#redirect' => array('staff/camp/registration', "year=$year"),
  );
  $form['paid_by'] = array(
  '#type' => 'textfield',
    '#title' => 'Paid by',
    '#size' => 40,
    '#autocomplete_path' => 'staff/js/autocomplete/contact',
    '#default_value' => $parent['display_name'] . ' (ID#' . $cid.')'
  );
  $form['amount'] = array(
    '#type' => 'textfield',
    '#title' => 'Payment Amount',
    '#field_prefix' => '$',
    '#size' => 8,
    '#prefix' => '<div class="subset">',
  );
  $form['method'] = array(
    '#type' => 'select',
    '#title' => 'Payment Method',
    '#options' => array(4=> 'Check', 3=> 'Cash'),
  );
  $form['check_no'] = array(
    '#type' => 'textfield',
    '#title' => 'Check Number',
    '#size' => 8,
    '#suffix' => '</div>',
  );
  $form['date'] = array(
    '#type' => 'date',
    '#title' => 'Date',
  );
  $form['email'] = array(
    '#type' => 'checkbox',
    '#title' => 'Send Acknowledgement',
    '#default_value' => TRUE
  );
  $form['submit-payment'] = array(
    '#type' => 'submit',
    '#value' => 'Save Payment',
  );
  return $form;
}

/**
 * Validate handler for staff to enter camp payment
 */
function woolman_camp_add_payment_validate($form, &$form_state) {
  $vals = $form_state['values'];
  if ($vals['method'] == 4 && !$vals['check_no']) {
    form_set_error('check_no', 'Please enter check #.');
  }
  if (!$vals['amount'] || !is_numeric($vals['amount'])) {
    form_set_error('amount', 'Please enter dollar amount paid.');
  }
}

/**
 * Submit handler for staff to enter camp payment
 */
function woolman_camp_add_payment_submit($form, &$form_state) {
  $reg = $form_state['storage']['reg'];
  $year = $form_state['storage']['year'];
  $parent = $form_state['storage']['parent'];
  unset($form_state['storage']);
  $vals = $form_state['values'];
  $payer = woolman_split_name_id($vals['paid_by']);
  if (!$payer['id'] || !is_numeric($payer['id'])) {
    $payer['id'] = $parent['contact_id'];
  }

  $params = array(
    'contribution_type_id' => 5,
    'contact_id' => $payer['id'],
    'receive_date' => woolman_date_array_to_string($vals['date']),
    'total_amount' => $vals['amount'],
    'payment_instrument_id' => $vals['method'],
    'source' => "$year",
    'check_number' => $vals['check_no'],
    'contribution_status_id' => 1,
  );
  if ($parent['contact_id'] != $payer['id']) {
    $params['soft_credit_to'] = $parent['contact_id'];
  }
  if ($vals['email']) {
    $params['receipt_date'] = date('YmdHis');
  }
  woolman_civicrm_api('contribution', 'create', $params);
  woolman_camp_update_totals($reg, $vals['amount']);

  $children = array();
  foreach ($reg['campers'] as $camper) {
    $children[] = $camper['first_name'];
  }
  if ($vals['email']) {
    $children = implode(' and ', $children);
    $to = '"' . $parent['display_name'] . '" <' . $parent['email'] . '>';
    $from = '"Woolman Camps" <camp@woolman.org>';
    $message['subject'] = 'Payment Received';
    $message['body'] = "Hello {$parent['nick_name']}\n\nThis is to acknowledge that we have received your payment of \${$vals['amount']}. Camp will be here before you know it, so double-check that you have everything on the packing list: http:// Camp.woolman.org/packing-list\n\nAs always, please don't hesitate to contact us with any questions.\n\nWe're looking forward to a great summer with $children!\n\nSincerely,\nYour Friendly Woolman Camp Staff\n530-273-3183\nhttp:// Camp.woolman.org";
    $message['to_cid'] = $parent['contact_id'];
    drupal_mail('woolman_website', 'create_activity', $to, language_default(), $message, $from);
    drupal_set_message('An email acknowledgment has been sent to ' . $parent['display_name']);
  }
}

/**
 * Form builder for staff to manage discounts
 */
function woolman_camp_manage_discount($form_state) {
  $form = array(
    '#redirect' => array('staff/camp/registration'),
  );
  if (empty($form_state['storage']['cid'])) {
    drupal_set_title('Manage Discounts');
    $form['cid'] = array(
      '#type' => 'textfield',
      '#size' => 50,
      '#title' => 'Parent Name',
      '#description' => '<em>Note: This parent needs to exist in the database before you can add/edit their discounts. Ask them to fill out the basic "family info" form if they haven\'t already.</em>',
      '#required' => TRUE,
      '#autocomplete_path' => 'staff/js/autocomplete/Individual',
    );
    $now = woolman_camp_next_year();
    $form['year'] = array(
      '#type' => 'select',
      '#title' => 'Year',
      '#description' => 'Which year of camp do you want to add/edit discounts for this person?<br />(select ' . ($now+1) . ' to credit any over-payments from this year)',
      '#options' => drupal_map_assoc(range($now - 2, $now + 1)),
      '#default_value' => $now,
      '#suffix' => '<p style="height:100px;">&nbsp;</p>',
    );
    $form['submit-cid'] = array(
      '#type' => 'submit',
      '#value' => 'Continue',
    );
  }
  else {
    $contact = woolman_split_name_id($form_state['storage']['cid']);
    $form_state['storage']['cid'] = $cid = $contact['id'];
    $year = $form_state['storage']['year'];
    drupal_set_title($year . ' Discounts for ' . woolman_name('display', $cid));
    $form['discounts'] = array(
      '#tree' => TRUE,
      '#prefix' => '<table><thead><tr><th>Type</th><th>Amount</th><th> </th></tr></thead><tbody>',
      '#suffix' => '</tbody></table>',
    );
    $fields = array(
      '#prefix' => '<tr>',
      '#suffix' => '</tr>',
      'type' => array(
        '#prefix' => '<td>',
        '#suffix' => '</td>',
        '#type' => 'select',
        '#options' => array('' => '- Delete -') + woolman_camp_discount_types(),
      ),
      'amount' => array(
        '#prefix' => '<td>',
        '#suffix' => '</td>',
        '#type' => 'textfield',
        '#size' => 10,
      ),
      'date' => array(
        '#prefix' => '<td>',
        '#suffix' => '</td>',
        '#value' => '<span class="today">Given today</span>',
      ),
    );
    $db = db_query('SELECT * FROM {woolman_camp_discounts} WHERE year = %d AND cid = %d', $year, $cid);
    $form_state['storage']['discounts'] = array();
    while ($row = db_fetch_object($db)) {
      $form_state['storage']['discounts'][$row->id] = (array) $row;
      $form['discounts'][$row->id] = $fields;
      $form['discounts'][$row->id]['type']['#default_value'] = $row->type;
      $form['discounts'][$row->id]['amount']['#default_value'] = $row->amount;
      $form['discounts'][$row->id]['date']['#value'] = '<span class="date-info">Given on ' . woolman_camp_date_format($row->date) . '</span>';
    }
    $form['discounts'][0] = $fields;
    $form['discounts'][0]['amount']['#prefix'] = '<td class="new">';
    $form['discounts'][0]['type']['#options'][''] = 'Add New...';
    $form['clear'] = array('#value' => '<p style="clear:both; height:10px;"></p>');
    $form['submit-save'] = array(
      '#type' => 'submit',
      '#value' => 'Save Changes',
    );
  }
  return $form;
}

/**
 * Submit handler for staff to manage discounts
 */
function woolman_camp_manage_discount_submit($form, &$form_state) {
  if (empty($form_state['storage'])) {
    $form_state['rebuild'] = TRUE;
    $form_state['storage'] = $form_state['values'];
    return;
  }
  $cid = $form_state['storage']['cid'];
  $year = $form_state['storage']['year'];
  foreach ($form_state['values']['discounts'] as $id => $d) {
    if ($id) {
      $existing = $form_state['storage']['discounts'][$id];
      if (!($d['type'] && $d['amount'])) {
        db_query("DELETE FROM {woolman_camp_discounts} WHERE id = $id");
      }
      elseif (!($d['type'] == $existing['type'] && $d['amount'] == $existing['amount'])) {
        db_query("UPDATE {woolman_camp_discounts}
        SET amount = '%s', type = '%s'
        WHERE id = $id
        ", $d['amount'], $d['type']);
      }
    }
    elseif ($d['type'] && $d['amount']) {
      db_query("INSERT INTO {woolman_camp_discounts}
      (cid, amount, date, year, type) VALUES
      ($cid, '%s', CURDATE(), $year, '%s')
      ", $d['amount'], $d['type']);
    }
  }
  unset($form_state['storage']);
  drupal_set_message($year . ' discounts saved for ' . woolman_name('full', $cid));
}

/**
 * Form builder for staff to add discounts
 */
function woolman_camp_add_discount(&$form_state, $id, $reg, $year) {
  $programs = woolman_camp_event_types();
  $form_state['storage']['year'] = $year;
  $form_state['storage']['parent'] = $parent = woolman_contact_get($id);
  $form = array('#redirect' => array('staff/camp/registration', "year=$year"));
  $form['type'] = array(
    '#prefix' => '<table><caption><strong>Fees for ' . $parent['display_name'] . '</strong></caption><tbody>',
    '#type' => 'select',
    '#options' => array('' => '- Select Type -') + woolman_camp_discount_types(),
    '#required' => TRUE,
    '#suffix' => '</td>'
  );
  $form['amount'] = array(
    '#prefix' => '<td>',
    '#type' => 'textfield',
    '#field_prefix' => '-$',
    '#size' => 5,
    '#default_value' => 0,
    '#attributes' => array('onkeyup' => 'campStaff.discountCalc(this.value);', 'onblur' => 'campStaff.discountCalc(this.value);'),
    '#suffix' => '</td></tr>',
    '#required' => TRUE,
  );
  foreach ($reg['sessions'] as $type => $sessions) {
    if ($s = $sessions['count']) {
      $form['type']['#prefix'] .= '<tr><td>' . $s . ' Session' . ($s == 1 ? '' : 's') . ' of ' . $programs[$type]['name'] . ':</td><td>$' . $sessions['price'] . '</td></tr>';
    }
  }
  if ($w = $reg['shuttles']) {
    $form['type']['#prefix'] .= '<tr><td>' . $w .' Shuttle Ride' . ($w==1?'':'s') . ':</td><td>$' . $reg['shuttle_price'] . '</td></tr>';
  }
  foreach ($reg['discounts'] as $title => $info) {
    $form['type']['#prefix'] .= '<tr style="font-style:italic;"><td>' . $title.':</td><td>-$' . $info['amount'] . '</td></tr>';
  }

  $form['type']['#prefix'] .= '<tr><td>';

  $form['amount']['#suffix'] .= '<tr style="font-weight:bold;"><td>Fees Minus Discounts:</td><td>$<span id="discount-form-subtotal">' . ($reg['total_fees']-$reg['total_discount']) . '</span></td></tr>';

  $form['amount']['#suffix'] .= '<tr><td>Paid so far by ' . $parent['display_name'] . ':</td><td>$<span id="discount-form-paid">' . $reg['total_paid'] . '</span></td></tr>';

  $form['amount']['#suffix'] .= '<tr style="font-weight:bold;"><td>Balance Due:</td><td>$<span id="discount-form-balance">' . ($reg['total_fees']-$reg['total_discount']-$reg['total_paid']) . '</span></td></tr></tbody></table>
  <div id="discount-form-base" style="display:none;">' . ($reg['total_fees']-$reg['total_discount']) . '</div>';

  $form['submit-payment'] = array(
    '#type' => 'submit',
    '#value' => 'Save'
  );
  return $form;
}

/**
 * Validate handler for staff to add discounts
 */
function woolman_camp_add_discount_validate($form, &$form_state) {
  $vals = $form_state['values'];
  if ($vals['type'] && !is_numeric($vals['amount'])) {
    form_error($form['amount'], 'Sorry, ' . $vals['amount'] . ' is not a valid amount');
  }
}

/**
 * Submit handler for staff to add discounts
 */
function woolman_camp_add_discount_submit($form, &$form_state) {
  $year = $form_state['storage']['year'];
  $parent = $form_state['storage']['parent'];
  unset($form_state['storage']);
  $vals = $form_state['values'];
  db_query('INSERT INTO {woolman_camp_discounts} (cid,amount,date,year,type) VALUES (' . $parent['contact_id'] . ',' . $vals['amount'] . ',CURDATE(),' . $year.',\'' . $vals['type'] . '\')');
  drupal_set_message('Added ' . $vals['type'] . ' discount of $' . $vals['amount'] . ' for ' . $parent['display_name']);
}

/**
 * Fee report
 */
function woolman_camp_fee_report($year) {
  $sessions = woolman_camp_get_sessions($year);
  $thead = array(
    'Parent',
    'Campers',
    'Weeks',
    'Register Date',
    'Base Fees',
    'Discounts',
    'Adjusted Fees',
    'Paid To Date',
    'Outstanding',
  );
  // Get parents
  $query = db_query("
    SELECT n.parent_15 AS cid, c.display_name
    FROM {civicrm_participant} p, {civicrm_value_registration_questions_7} n, {civicrm_contact} c
    WHERE n.entity_id = p.id AND c.id = n.parent_15 AND p.event_id IN (" . implode(',', array_keys($sessions)) . ")
    AND p.status_id IN (SELECT id FROM {civicrm_participant_status_type} WHERE is_counted)
    GROUP BY n.parent_15
    ORDER BY c.sort_name");
  $total_campers = $total_weeks = $total_fees = $total_discount = $total_paid = $total_shuttle = 0;
  while ($c = db_fetch_array($query)) {
    $r = woolman_camp_calc_fees($c['cid'], $year);
    $tbody[] = array(
      $c['display_name'],
      count($r['campers']),
      $r['weeks'],
      date('M j Y', strtotime($r['register_date'])),
      '$' . $r['total_fees'],
      '$' . $r['total_discount'],
      '$' . ($r['total_fees']-$r['total_discount']),
      '$' . $r['total_paid'],
      '$' . ($r['total_fees']-$r['total_discount']-$r['total_paid'])
    );
    $total_campers += count($r['campers']);
    $total_weeks += $r['weeks'];
    $total_fees += $r['total_fees'];
    $total_discount += $r['total_discount'];
    $total_paid += $r['total_paid'];
    $total_shuttle += $r['shuttles'] * $r['rates']['shuttle_price'];
  }
  $tbody[] = array(
    'data' => array(
      'TOTAL',
      $total_campers,
      $total_weeks,
      '',
      '$' . number_format($total_fees),
      '$' . number_format($total_discount),
      '$' . number_format(($total_fees-$total_discount)),
      '$' . number_format($total_paid),
      '$' . number_format(($total_fees-$total_discount-$total_paid))
    ),
    'style' => 'font-weight:bold;');
  return theme('table', $thead, $tbody) . '<p style="font-weight:bold;">Average Tuition: $' . number_format((($total_fees-$total_discount-$total_shuttle)/$total_weeks)) . ' per week.</p>';
}

/**
 * Callback used for in-place editing
 */
function woolman_camp_js() {
  $output = '';
  if (!empty($_GET['op'])) {
    civicrm_initialize();
    switch ($_GET['op']) {

      case 'change_status':
        if (empty($_GET['pid']) || empty($_GET['sid'])) {
          break;
        }
        foreach (explode(',', $_GET['pid']) as $pid) {
          $result = woolman_civicrm_api('participant', 'create', array(
            'id' => $pid,
            'status_id' => $_GET['sid'],
          ));
          if (!empty($result['is_error'])) {
            exit('"error"');
          }
        }
        $sql = 'SELECT LOWER(class) FROM civicrm_participant_status_type WHERE id = %1';
        $output = CRM_Core_DAO::singleValueQuery($sql, array(1 => array($_GET['sid'], 'Integer')));
        break;

    }
  }
  exit('"' . $output . '"');
}

/**
 * Bar Graph of campers by age
 */
function woolman_camp_staff_graph_session($session) {
  require_once DRUPAL_ROOT . '/sites/all/libraries/jpgraph/src/jpgraph.php';
  require_once DRUPAL_ROOT . '/sites/all/libraries/jpgraph/src/jpgraph_bar.php';
  $programs = woolman_camp_event_types();
  $program = $programs[$session['event_type_id']];
  $range = $program['age_max'] - $program['age_min'] + 1;
  $boys = $girls = array_fill($program['age_min'], $range, 0);
  $size = $range * 50;
  $size = $size > 240 ? $size : 240;
  $max = 8;

  foreach ($session['campers'] as $c) {
    if ($c['status'] == 'On waitlist' || $c['class'] == 'Negative') {
      continue;
    }
    $sex = ($c['gender_id'] == 1 ? 'girls' : 'boys');
    // When adding a kid outside the standard age range, do it for both genders
    if (!isset($boys[$c['age']])) {
      $boys[$c['age']] = $girls[$c['age']] = 0;
      ksort($boys);
      ksort($girls);
    }
    if (++${$sex}[$c['age']] > $max) {
      $max = ${$sex}[$c['age']];
    }
  }

  $graph = new Graph($size, 240);
  $graph->SetScale('textlin');
  $graph->ygrid->SetWeight(0);
  $graph->yaxis->scale->SetAutoMax($max);
  $graph->img->SetAntiAliasing(FALSE);
  $graph->SetMargin(22, 8, 25, 48);
  $graph->SetMarginColor('#FAFFDC');
  $graph->title->Set('Registered Campers*');
  $graph->title->SetFont(FF_DV_SANSSERIF,FS_BOLD);
  $graph->xaxis->SetTickLabels(array_keys($girls));
  $graph->yaxis->SetTickLabels(array(0,1,2,3,4,5));
  $graph->SetFrame(true,'#D0E7A8',1);

  $gplot = new BarPlot(array_values($girls));
  $gplot-> setLegend('Girls');
  $gplot-> SetFillColor('#6FFF5E');
  $bplot = new BarPlot(array_values($boys));
  $bplot-> setLegend('Boys');
  $bplot-> SetFillColor('#817BFF');

  $gbplot = new GroupBarPlot(array($bplot, $gplot));

  $graph->Add($gbplot);
  $graph->Stroke('sites/default/files/woolman_graphs/session'  . $session['id'] . '.png');
}

/**
 * Line Graph of campers by registration date
 */
function woolman_camp_staff_graph_registration($year) {
  require_once DRUPAL_ROOT . '/sites/all/libraries/jpgraph/src/jpgraph.php';
  require_once DRUPAL_ROOT . '/sites/all/libraries/jpgraph/src/jpgraph_line.php';
  if ($year < 2012) {
    return;
  }
  $prev_year = $year - 1;
  $data = $y_axis = array($prev_year => array(), $year => array());
  $query = db_query(
    "SELECT YEAR(ev.start_date) as y, MONTH(par.register_date) as m, DAY(par.register_date) as d, DATEDIFF(ev.end_date, ev.start_date) AS days, DATE(l.modified_date) as modified_date, par.status_id
    FROM {civicrm_participant} par
    INNER JOIN {civicrm_event} ev ON par.event_id = ev.id
    INNER JOIN {civicrm_contact} con ON con.id = par.contact_id
    LEFT JOIN (SELECT entity_id, MAX(modified_date) as modified_date FROM {civicrm_log} WHERE data = 'Cancelled' AND entity_table = 'civicrm_participant' GROUP BY entity_id) l ON l.entity_id = par.id
    WHERE ev.event_type_id IN(8, 9) AND YEAR(ev.start_date) IN ($year, $prev_year) AND ev.is_active = 1 AND ev.is_template <> 1 AND par.is_test <> 1 AND con.is_deleted = 0 AND (par.status_id IN (1,2,8,13) OR (par.status_id = 4 AND l.modified_date IS NOT NULL))
  ");
  while ($row = db_fetch_object($query)) {
    $y = (int) $row->y;
    $m = (int) $row->m;
    $d = (int) $row->d;
    $w = (int) round($row->days / 7);
    $data[$y][$m][$d] = woolman_aval($data[$y], "$m:$d", 0) + $w;
    // Subtract cancelled participants on cancellation date
    if ($row->status_id == 4) {
      list(, $m, $d) = explode('-', $row->modified_date);
      $data[$y][intval($m)][intval($d)] = woolman_aval($data[$y], "$m:$d", 0) - $w;
    }
  }
  $this_month = (int) date('m');
  $today = (int) date('d');
  foreach ($data as $y => $info) {
    $x = $c = 0;
    for ($m = 1; $m <= 8; ++$m) {
      // Get # of days in month; account for leap-year
      $days = date('t', strtotime("$year-$m"));
      if ($days < date('t', strtotime("$prev_year-$m"))) {
        $days = date('t', strtotime("$prev_year-$m"));
      }
      for ($d = 1; $d <= $days; $d++) {
        $c += woolman_aval($info, "$m:$d", 0);
        $y_axis[$y][] = $c;
        if ($y < $year) {
          ++$x;
          if ($d == 1) {
            $x_labels[$x] = date('M', strtotime("$y-$m"));
          }
        }
        if ($y == $year && $d >= $today && $m === $this_month) {
          break 3;
        }
        elseif ($m === 8) {
          break;
        }
      }
    }
  }
  $graph = new Graph(350, 150);
  $graph->SetScale('linlin', 0, 0, 0, count($y_axis[$prev_year]) + 1);
  $graph->legend->SetAbsPos(214, 91, 'left','top');
  $graph->img->SetAntiAliasing(FALSE);
  $graph->xaxis->SetPos('min');
  $graph->xaxis->SetMajTickPositions(array_keys($x_labels), array_values($x_labels));
  $graph->xaxis->HideTicks(TRUE, FALSE);
  $graph->SetMargin(30, 10, 10, 0);
  $graph->SetMarginColor('#E8F2E0');
  $graph->SetFrame(true, '#FFFFFF', 0);
  foreach ($y_axis as $y => $info) {
    $p = new LinePlot($info, array_keys($info));
    $p->setLegend($y);
    $graph->Add($p);
  }
  $graph->Stroke('sites/default/files/woolman_graphs/year-' . $year . '.png');
}
