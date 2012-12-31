<?php

module_load_include('inc', 'woolman_camp', 'woolman_camp_forms');

/**
 * Camp landing page callback
 * Displays general information about registration
 */
function woolman_camp_landing() {
  drupal_add_css(drupal_get_path('module', 'woolman_camp') . '/css/camp_landing.css');
  $year = woolman_camp_next_year();
  $programs = woolman_camp_event_types();
  $welcome = '';
  $form_link = '>> Click here to register';
  $form_url = '~camp/register/form';
  if (user_is_logged_in()) {
    $welcome = '<div>Welcome back, ' . woolman_name('nick') . '</div>';
    $act = woolman_camp_get_reg_act();
    if ($act['status_id'] == 4) {
      $welcome .= '<div><em>Your registration is in progress</em></div>';
      $form_link = '>> Click here to continue your registration';
    }
    elseif ($act['status_id'] == 2) {
      $welcome .= "<div><em>You've registered for camp {$year}!</em></div>";
      $form_link = '>> Click here to view your registration, make changes, & pay tuition.';
      $form_url = '~camp/register/status';
    }
  }
  $body = '
    <h4>Registration Process</h4>
    <ul class="styled star">
      <li>Enter or update your family info.</li>
      <li>Decide which sessions your children will attend.</li>
      <li>Fill-out their emergency medical info.</li>
      <li>Complete your registration by paying $100 deposit per week (we also accept cash and checks if you prefer not to pay online).</li>
      <li>That\'s it! We\'ll send you an email to confirm your registration.</li>
    </ul>
    <p>More information can be found on our <a href="/parents/questions">FAQ</a> page. Prices are here: <a href="/dates-rates">Dates &amp; Rates</a>. Need to know more? Please don\'t hesitate to <a href="/contact?c=3">contact us</a>.</p>
  ';
  $session_data = woolman_camp_get_sessions($year);
  if (!$session_data) {
    return '
      <h2>Online Registration for ' . $year . '</h2>
      <p>We have not yet started registering for next year. Upcoming dates and rates will be announced soon.</p>' .
      $body;
  }
  $rates = woolman_camp_get_rates($year);
  $sessions = array_fill_keys(array_keys($programs), '');
  foreach ($session_data as $s) {
    $type = $s['event_type_id'];
    $percent = $s['campers'] / $s['max_participants'];
    if (substr($s['start_date'], 0, 10) < date('Y-m-d')) {
      if (substr($s['end_date'], 0, 10) <= date('Y-m-d')) {
        $status = '<span class="closed">Ended.</span>';
      }
      else {
        $status = '<span class="closed">Already started.</span>';
      }
    }
    elseif ($percent >= 1) {
      $status = '<span class="closed">FULL. Waitlist only.</span>';
    }
    else {
      if ($percent >= .75) {
        $status = '<span class="full">Almost full.</span>';
      }
      elseif ($percent >= .50) {
        $status = '<span class="filling">Filling fast.</span>';
      }
      else {
        $status = '<span class="open">Space available.</span>';
      }
      if ($rates['gender_balance'] !== '') {
        if ($percent < 1 && $s['boys'] >= ($rates['gender_balance'] / 100 * $s['max_participants'])) {
          $status .= '<br /><span class="full">Open for girls, waitlist for boys.</span>';
        }
        elseif ($percent < 1 && $s['girls'] >= ($rates['gender_balance'] / 100 * $s['max_participants'])) {
          $status .= '<br /><span class="full">Open for boys, waitlist for girls.</span>';
        }
        elseif ($percent < 1) {
          $status .= '<br /><span class="open">Open for boys and girls.</span>';
        }
      }
    }
    $sessions[$type] .= '
      <div class="session">
        <h5 class="session-title">' . $s['title'] . '</h5>
        <div class="session-dates">
          ' . woolman_camp_date_format($s['start_date'], $s['end_date']) . '
        </div>
        <div class="session-status">
          ' . $status . '
        </div>
      </div>
    ';
  }
  $content = '<div id="session-outer"> <h3>This Year\'s Sessions</h3>';
  $classes = array('even' => 'odd', 'odd' => 'even');
  $class = 'odd';
  foreach ($programs as $type => $program) {
    if ($sessions[$type]) {
      $content .= "
        <div class='session-wrapper program-$type $class'>
          <h4>{$program['name']}</h4>
          <p class='session-ages'>Ages {$program['age_min']}-{$program['age_max']}</p>
          {$sessions[$type]}
        </div>";
      $class = $classes[$class];
    }
  }
  $content .= '
    </div>
    <div id="registration-info">
      <h2>Online Registration: Now Open for ' . $year . '</h2>' .
      $welcome . l($form_link, $form_url, array('alias' => TRUE)) . $body . '
    </div>';

  return $content;
}

/**
 * REGISTRATION FORM WORKFLOW CONTROLLER
 * Returns appropriate form/page
 * Based on where the user is in the registration process
 * Called from hook menu for both ~camp/register/form and ~camp/register/status
 */
function woolman_camp_page() {
  drupal_add_css(drupal_get_path('module', 'woolman_camp') . '/css/camp_forms.css');

  if (!woolman_camp_get_sessions(woolman_camp_next_year())) {
    drupal_goto('~camp/register');
  }

  if (!user_is_logged_in()) {
    if (arg(2) == 'status') {
      drupal_set_title('Please Log In');
      return drupal_access_denied();
    }
    return '<h3>Welcome to Camp Woolman Registration ' . woolman_camp_next_year() . '</h3>
    <p><strong>Hey there!</strong> We think you\'re going to like our spiffy online-registration system.</p>
    <h4>Online Registration is</h4>
    <ul class="styled star">
      <li><strong>Quick and easy.</strong> No more filling out the same info on paper forms year after year.</li>
      <li><strong>Better for the planet.</strong> Our website is printed on recycled electrons :)</li>
      <li><strong>Free!</strong> That\'s right, no more application fees for camp!</li>
    </ul>
    <hr />
    <p>In order to get started, please ' .
    l('click here to log in', 'user', array('query' => 'destination=camp/register'))
    . '. If you don\'t yet have a Woolman.org account, you can create one right here:</p>
    <h4 class="underline">Create New Account</h4>
    <p><em><strong>Note: This form is for parents/guardians.</strong> Please enter your own information below, we\'ll ask about your camper(s) in the next step.</em></p>'
    .drupal_get_form('user_register');
  }
  // Look for registration activity
  $act = woolman_camp_get_reg_act();

  // Registration completed
  if (woolman_aval($act, 'status_id') == 2) {
    if (arg(2) == 'form') {
      drupal_goto('~camp/register/status');
    }
    return drupal_get_form('woolman_camp_nav_form', 3) . woolman_camp_status();
  }
  // Registration in progress
  if (arg(2) == 'status') {
    drupal_goto('~camp/register/form');
  }
  if (woolman_aval($act, 'status_id') == 4) {
    return drupal_get_form('woolman_camp_nav_form', 2) . drupal_get_form('woolman_camp_registration_form');
  }
  // Begin registration
  woolman_camp_update_reg_act(6);

  // Look for "how did you hear about us" activity
  $query = db_query('
    SELECT id
    FROM {civicrm_activity}
    WHERE source_contact_id = %d AND activity_type_id = 53 AND is_deleted <> 1 AND is_test <> 1', woolman_user_cid()
  );
  if (db_fetch_array($query)) {
    return drupal_get_form('woolman_camp_nav_form', 1) . drupal_get_form('woolman_camp_family_form');
  }

  // Check for prior camp registrations
  list($children , $spouse, $e_contact, $past_reg) = woolman_camp_fetch_relations();
  if ($past_reg) {
    return drupal_get_form('woolman_camp_nav_form', 1) . drupal_get_form('woolman_camp_family_form');
  }

  return drupal_get_form('woolman_camp_nav_form', 0)
  . '<p><em>Note: Camp registration forms should be completed by parents, guardians, or other responsible adults. Please ensure that you are logged-in as the parent/guardian and not the camper.</em></p>'
  . drupal_get_form('woolman_camp_first_time_form');
}

/**
 * Once the initial forms are complete, we show the registration and payment status
 */
function woolman_camp_status() {
  popups_add_popups();
  drupal_add_js(drupal_get_path('module', 'woolman_camp') . '/js/status_page.js');
  $year = woolman_camp_next_year();
  $shuttles = woolman_camp_get_shuttle($year, FALSE);
  $programs = woolman_camp_event_types();
  $reg = woolman_camp_calc_fees(woolman_user_cid(), $year);
  $output = ' <h2>Registration Status</h2>
              <p>This page shows you the current status of your registration. As you complete forms, have applications accepted, and pay fees, you\'ll be able to see your registration status updated here. <em>You may wish to bookmark this page.</em></p>
              <div id="reg-status-page">
                <div id="campers">
                  <p><h4>Important Information</h4>
                    <ul class="styled star">
                      <li><a href="/packing-list">Packing List</a></li>
                      <li><a href="/shuttle">Shuttle Info</a></li>
                      <li><a href="/parents/questions">FAQs for Parents</a></li>
                    </ul>
                  </p>';
  foreach ($reg['campers'] as $cid => $c) {
    $med = 'Click Here to Complete ' . $c['first_name'] .'\'s Medical Form';
    $medclass = 'incomplete';
    if ($c['medform_date'] > 0) {
      $med = 'Medical Form Completed';
      $medclass = 'done';
    }
    $output .= '<div class="camper-info">
                  <p class="medform ' . $medclass . '">'. l($med, '~camp/register/medical/' . $cid, array('alias' => TRUE, 'attributes' => array('class' => 'popups-form'))) .'</p>
                  <h4 class="camper-name">' . $c['first_name'] . ' ' . $c['last_name'] . '</h4>
                  <div class="sessions">';
    foreach ($c['sessions'] as $s) {
      $output .=  ' <div class="session">
                      <div style="font-weight:bold;">' . $s['title'] . '</div>
                      <div>'. woolman_camp_date_format($s['start_date'], $s['end_date']) . '</div>
                      <div><strong>Registration Status:</strong> ' . $s['message'] .'</div>
                    </div>';
    }
    if ($shuttles['coming'] || $shuttles['going']) {
      $output .= '<div class="session shuttle">
                    <div style="font-weight:bold;">Bay Area Shuttle</div>';
      $shuttle = '';
      if (isset($c['shuttle'])) {
        foreach ($c['shuttle'] as $sh) {
          $shuttle .= '<li>' . $sh['title'] . '</li>';
        }
      }
      if ($shuttle) {
        $output .= l('View/Edit Shuttle Details', '~camp/register/shuttle', array('alias' => TRUE, 'attributes' => array('class' => 'popups-form'))) . '<ul>' . $shuttle . '</ul>';
      }
      else {
        $output .= l('Click here to sign up for the shuttle', '~camp/register/shuttle', array('alias' => TRUE, 'attributes' => array('class' => 'popups-form')));
      }
      $output .= '</div></div>';
    }
    $output .= '<div style="clear:both;"></div></div>';
  }
  $c = count($reg['campers']);
  $output .=  ' </div>
                  <div id="payment-info">
                    <h3>Payment Info</h3>
                    <p>You have <strong>' . woolman_num_to_words($c) . '</strong> camper' . ($c == 1 ? '' : 's');
  if ($sessions = $reg['total_approved'] + $reg['total_pending']) {
    $output .= ' registered for <strong>' . woolman_num_to_words($sessions) . '</strong> session' . ($sessions <= 1 ? '' : 's') . ' at camp';
    if (!empty($reg['total_waitlist'])) {
      $output .= ', and you\'re on the waitlist for an additional <strong>' . woolman_num_to_words($reg['total_waitlist']) . '</strong> session' . ($reg['total_waitlist'] == 1 ? '' : 's');
    }
    $output .= '.</p>';
  }
  else {
    $output .= '. They\'re on the waitlist for a total of <strong>' . woolman_num_to_words($reg['total_waitlist']) . '</strong> session' . ($reg['total_waitlist'] == 1 ? '' : 's') . ' at camp.</p>';
  }
  if ($reg['total_fees'] - $reg['total_discount'] > $reg['total_paid']) {
    $output .= '<p>';
    if ($reg['deposit'] > $reg['total_paid']) {
      $output .= 'Deposit fees ($' . $reg['rates']['deposit'] . ' per week) should be paid right away to reserve your place at camp. ';
    }
    $output .= 'Full tuition is due on the first day of camp.</p>';
  }
  $output .= '<table><caption><strong>Your Camp Fees</strong></caption><tbody>';
  foreach ($reg['sessions'] as $id => $camp) {
    if ($camp['count']) {
      $output .= '<tr><td>' . $camp['count'] . ' Session' . ($w == 1 ? '' : 's') . ' of ' . $programs[$id]['name'] .':</td><td class="p">$' . $camp['price'] . '</td></tr>';
    }
  }
  if ($w = $reg['shuttles']) {
    $output .= '<tr><td>' . $w . ' Shuttle Ride' . ($w == 1 ? '' : 's') . ':</td><td class="p">$' . $reg['shuttle_price'] . '</td></tr>';
  }
  foreach ($reg['discounts'] as $title => $info) {
    $output .= '<tr style="font-style:italic;"><td>' . $title.':</td><td class="p">-$' . $info['amount'] . '</td></tr>';
  }

  $output .= '<tr style="font-weight:bold;"><td>Your Total:</td><td class="p">$' . ($reg['total_fees'] - $reg['total_discount']) . '</td></tr>';

  $output .= '</tbody></table><table><caption><strong>Your Payments</strong></caption><tbody>';

  $output .= '<tr><td>Deposit Paid:</td><td class="p">$' . ($reg['deposit']<=$reg['total_paid']?$reg['deposit']:$reg['total_paid']) . ' of $' . $reg['deposit'] . '</td></tr>';

  $output .= '<tr><td>Total Paid:</td><td class="p">$' . $reg['total_paid'] . ' of $' . ($reg['total_fees']-$reg['total_discount']) . '</td></tr>';

  $output .= '<tr style="font-weight:bold;"><td>Total Balance Due:</td><td class="p">$' . ($reg['total_fees']-$reg['total_discount'] - $reg['total_paid']) . '</td></tr>';

  $output .= '</tbody></table>';
  if ($reg['total_waitlist']) {
    $output .= '<p>(waitlisted sessions are not included in this total; you do not need to pay a deposit to be on the waitlist)</p>';
  }
  if ($reg['total_pending']) {
    $output .= '<p>Note: <em>we recommend waiting until all your applications have been approved before paying your deposit.</em></p>';
  }
  if (($reg['total_fees']-$reg['total_discount']-$reg['total_paid'])<=0) {
    $output .= '<p>Your fees are paid in full. Thank you!</p>';
  }
  else {
    if ($reg['deposit'] <= $reg['total_paid']) {
      $output .= '<p>We have received your deposit payment, and your space at camp is reserved. You may pay your remaining balance at any time.</p>';
    }
    elseif (substr($reg['register_date'], 0, 10) != date('Y-m-d')) {
      $output .= '<p>We have not yet received your deposit payment, so your space at camp is not reserved. Please feel free to <a href="/contact?c=3">contact us</a> about this if you have any questions.</p>';
    }
    $output .= l('Make online payment', 'civicrm/contribute/transact', array('alias' => TRUE, 'query' => array('reset' => 1, 'id' =>7), 'attributes' => array('class' => 'button-style'))) . 
    '<p style="font-size:.85em;"><strong>Or you can mail a check payable to "Camp Woolman" to:</strong><br />Camp Woolman<br />13075 Woolman Ln<br />Nevada City, CA 95959</p>';
  }
  $output .= '  </div>
              </div>';
  return $output;
}
