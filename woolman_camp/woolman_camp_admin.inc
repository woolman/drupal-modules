<?php

module_load_include('inc', 'woolman_camp', 'woolman_camp_utils');

/**
 * Admin form
 */
function woolman_camp_rates(&$form_state) {
  drupal_add_css(drupal_get_path('module', 'woolman_camp') .'/css/camp_staff.css', 'module', 'all', FALSE);
  drupal_add_js(drupal_get_path('module', 'woolman_camp') . '/js/staff_dates_rates.js');
  $year = woolman_camp_next_year();
  $prices = array();
  $programs = woolman_camp_event_types();
  $db = db_query('SELECT * FROM {woolman_camp_rates} WHERE year IN (%d,%d,%d)', $year-1, $year, $year+1);
  while ($row = db_fetch_array($db)) {
    $prices[$row['year']] = $row;
  }
  $sessions = array(
    $year => woolman_camp_get_sessions($year),
    ($year+1) => woolman_camp_get_sessions($year+1),
  );
  $form_state['storage']['sessions'] = $sessions;
  $form_state['storage']['prices'] = $prices;
  $form = array(
    '#redirect' => 'staff/camp/registration',
    '#prefix' => '<p>The camp year begins Sept 1st. On that date the website will automatically switch to show next year\'s dates and rates, and online registration will open.</p>',
  );
  while (isset($sessions[$year])) {
    $form["year_$year"] = array(
      '#type' => 'fieldset',
      '#title' => $year.' Dates &amp; Rates',
      '#tree' => TRUE,
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#group' => 'camp_admin_year',
      '#description' => "<h4 class='underline'>$year Dates &amp; Rates</h4>",
    );
    if (!($pricedefault = woolman_aval($prices, $year))) {
      $pricedefault = $prices[$year-1];
      $form["year_$year"]['#description'] = '<div class="messages">Rates for ' . $year . ' have not yet been saved. These are last year\'s rates -- you may wish to adjust them before pressing the save button.</div>';
    }
    $form["year_$year"]['sfc_multi_week_discount'] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#size' => 5,
      '#field_prefix' => '$',
      '#title' => 'SFC multi-week discount',
      '#description' => 'Discount per week for SFC campers coming for 2 or more weeks',
      '#default_value' => $pricedefault['sfc_multi_week_discount'],
    );
    $form["year_$year"]['family_discount'] = array(
      '#suffix' => '</div>',
      '#type' => 'textfield',
      '#size' => 5,
      '#field_prefix' => '$',
      '#title' => 'Family discount',
      '#description' => 'Per camper - applies to all programs',
      '#default_value' => $pricedefault['family_discount'],
    );
    $form["year_$year"]['early_discount'] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#size' => 5,
      '#field_prefix' => '$',
      '#title' => 'Early bird discount',
      '#description' => '(per camper)',
      '#default_value' => $pricedefault['early_discount'],
    );
    $form["year_$year"]['early_date'] = array(
      '#type' => 'date',
      '#title' => 'Early registration deadline',
      '#description' => 'Register by this date to get early bird discount',
      '#default_value' => woolman_date_string_to_array($pricedefault['early_date']),
      '#suffix' => '</div>',
    );
    $form["year_$year"]['deposit'] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#size' => 5,
      '#field_prefix' => '$',
      '#title' => 'Nonrefundable Deposit',
      '#description' => 'Per camper, per week',
      '#default_value' => $pricedefault['deposit'],
    );
    $form["year_$year"]['shuttle_price'] = array(
      '#type' => 'textfield',
      '#size' => 5,
      '#field_prefix' => '$',
      '#title' => 'Shuttle Price',
      '#description' => 'Price of a one-way shuttle ride',
      '#default_value' => $pricedefault['shuttle_price'],
    );
    $form["year_$year"]['gender_balance'] = array(
      '#type' => 'textfield',
      '#size' => 4,
      '#maxlength' => 3,
      '#field_suffix' => '%',
      '#title' => 'Gender Balance',
      '#description' => 'Max percentage of one gender allowed in a session',
      '#suffix' => '</div>',
      '#default_value' => $pricedefault['gender_balance'],
    );
    $camps = woolman_camp_sort_sessions($sessions[$year]);
    foreach ($programs as $c => $program) {
      $camp = array_values($camps[$c]);
      $name = $program['event_type'];
      $form["year_$year"][$c] = array(
        '#type' => 'fieldset',
        '#title' => "$year $name Sessions",
        '#description' => 'Ages ' . $program['age_min'] . '-' . $program['age_max'],
        '#attributes' => array('class' => 'session-selection'),
      );
      $form["year_$year"][$c]['number'] = array(
        '#type' => 'select',
        '#options' => range(0, 9),
        '#title' => 'Number of sessions',
        '#default_value' => count($camp),
        '#attributes' => array('class' => 'number-of-events'),
      );
      for ($s = 1; $s <= 9; ++$s) {
        $i = $s - 1;
        if ($s == 1) {
          $default_max = woolman_aval($camp, "1:max_participants", woolman_aval($sessions, ($year-1) . ":$c:1:max_participants"));
        }
        $form["year_$year"][$c][$s . '_title'] = array(
          '#prefix' => '<div class="session-dates">',
          '#type' => 'textfield',
          '#size' => 24,
          '#default_value' => woolman_aval($camp, "$i:title", $year . ' ' . trim(str_replace(array('-Camp', 'Camp'), '', $name)) . ' Session ' . $s),
          '#suffix' => '<hr />',
        );
        $form["year_$year"][$c][$s . '_start'] = array(
          '#prefix' => '<div class="subset">',
          '#type' => 'date',
          '#title' => 'Start',
          '#default_value' => woolman_date_string_to_array(woolman_aval($camp, "$i:start_date")),
        );
        $form["year_$year"][$c][$s . '_end'] = array(
          '#suffix' => '</div>',
          '#type' => 'date',
          '#title' => 'End',
          '#default_value' => woolman_date_string_to_array(woolman_aval($camp, "$i:end_date")),
        );
        $form["year_$year"][$c][$s . '_price'] = array(
          '#prefix' => '<div class="subset">',
          '#type' => 'textfield',
          '#size' => 8,
          '#maxlength' => 8,
          '#field_prefix' => '$',
          '#title' => 'Price',
          '#default_value' => !empty($camp[$i]) ? woolman_camp_event_price_get($camp[$i]) : woolman_aval($form["year_$year"], "$c:1_price:#default_value"),
        );
        $form["year_$year"][$c][$s . '_max'] = array(
          '#type' => 'textfield',
          '#size' => 2,
          '#maxlength' => 2,
          '#title' => 'Max Campers',
          '#suffix' => '</div></div>',
          '#default_value' => woolman_aval($camp, "$i:max_participants", $default_max),
        );
      }
    }
    $form_state['storage']['shuttles'][$year] = $shuttles = woolman_camp_get_shuttle($year, FALSE);
    foreach (array('coming' => 'Shuttles to Camp (start of session)', 'going' => 'Shuttles from Camp (end of session)') as $dir => $label) {
      $form["year_$year"][$dir] = array(
        '#type' => 'fieldset',
        '#title' => $label,
        '#attributes' => array('class' => 'shuttle-selection ' . $dir),
      );
      $form["year_$year"][$dir]['number'] = array(
        '#type' => 'select',
        '#options' => range(0, 9),
        '#title' => 'Number of Shuttles',
        '#default_value' => count($shuttles[$dir]),
        '#attributes' => array('class' => 'number-of-events'),
      );
      $existing = array_values($shuttles[$dir]);
      for ($s=1; $s<=9; ++$s) {
        $i = $s - 1;
        $form["year_$year"][$dir][$s . '_title'] = array(
          '#prefix' => '<div class="shuttle-times"><h4 class="underline">Shuttle ' . ($dir == 'coming' ? 'to Camp ' : 'Home ') . $s . '</h4><div class="subset">',
          '#type' => 'textfield',
          '#title' => 'Title',
          '#size' => 22,
          '#default_value' => woolman_aval($existing, "$i:title"),
          '#description' => 'Name the session(s) this shuttle serves',
        );
        $form["year_$year"][$dir][$s . '_date'] = array(
          '#type' => 'date',
          '#title' => 'Date',
          '#default_value' => woolman_date_string_to_array(woolman_aval($existing, "$i:start_date")),
        );
        $form["year_$year"][$dir][$s . '_max'] = array(
          '#type' => 'textfield',
          '#size' => 2,
          '#maxlength' => 2,
          '#title' => 'Seats',
          '#suffix' => '</div>',
          '#default_value' => woolman_aval($existing, "$i:max_participants"),
        );
        $form["year_$year"][$dir][$s . '_description'] = array(
          '#type' => 'textarea',
          '#title' => 'Description',
          '#rows' => 4,
          '#cols' => 40,
          '#description' => 'Include pickup/dropoff times, locations, and instructions',
          '#suffix' => '</div>',
          '#default_value' => woolman_aval($existing, "$i:description"),
        );
      }
    }
    $form["year_$year"]['submit' . $year] = array(
      '#type' => 'submit',
      '#prefix' => '<div class="clear-block"> </div>',
      '#value' => 'Save ' . $year.' Dates & Rates',
    );
    if (empty($prices[$year])) {
      break; 
    }
    ++$year;
  }
  if (module_exists('vertical_tabs')) {
    $form['#pre_render'][] = 'vertical_tabs_form_pre_render';
  }
  return $form;
}

/**
 * Admin form validation handler
 */
function woolman_camp_rates_validate($form, &$form_state) {
  $year = substr($form_state['clicked_button']['#value'], 5, 4);
  $vals = $form_state['values']["year_$year"];
  if ($vals['gender_balance'] !== '' && (!is_numeric($vals['gender_balance']) || $vals['gender_balance'] < 50 || $vals['gender_balance'] > 99)) {
    form_error($form["year_$year"]['gender_balance'], 'Gender balance must be between 50% and 99% or else left blank.');
  }
  foreach (woolman_camp_event_types() as $type => $program) {
    for ($s = 1; $s <= $vals[$type]['number']; ++$s) {
      foreach (array('title', 'price', 'max') as $req) {
        if (empty($vals[$type][$s . '_' . $req])) {
          $field = $form["year_$year"][$type][$s . '_' . $req];
          form_error($field, 'Required Field: ' . $field['#title']);
        }
      }
    }
  }
  foreach (array('coming', 'going') as $dir) {
    for ($s=1; $s<=$vals[$dir]['number']; ++$s) {
      foreach (array('title', 'max') as $req) {
        if (empty($vals[$dir][$s . '_' . $req])) {
          $field = $form["year_$year"][$dir][$s . '_' . $req];
          form_error($field, 'Required Field: ' . $field['#title']);
        }
      }
    }
  }
}

/**
 * Admin form submit handler
 */
function woolman_camp_rates_submit ($form, &$form_state) {
  $year = substr($form_state['clicked_button']['#value'], 5, 4);
  $vals = $form_state['values']["year_$year"];
  $vals['year'] = $vals['early_date']['year'] = $year;
  $vals['early_date'] = woolman_date_array_to_string($vals['early_date'], '-');
  if (empty($form_state['storage']['prices'][$year])) {
    drupal_write_record('woolman_camp_rates', $vals);
  }
  else {
    drupal_write_record('woolman_camp_rates', $vals, 'year');
  }
  $now = date('YmdHis');
  $sessions = woolman_camp_sort_sessions($form_state['storage']['sessions'][$year]);
  // Save camp sessions
  foreach (woolman_camp_event_types() as $type => $program) {
    $camp = array_values($sessions[$type]);
    for ($s = 1; $s <= $vals[$type]['number']; ++$s) {
      $i = $s - 1;
      $vals[$type][$s . '_start']['year'] = $vals[$type][$s . '_end']['year'] = $year;
      $params = array(
        'title' => $vals[$type][$s . '_title'],
        'event_type_id' => $type,
        'is_public' => 1,
        'max_participants' => $vals[$type][$s . '_max'],
        'start_date' => woolman_date_array_to_string($vals[$type][$s . '_start']) . '120000',
        'end_date' => woolman_date_array_to_string($vals[$type][$s . '_end']) . '120000',
        'is_active' => 1,
        'is_monetary' => 1,
        'contribution_type_id' => 5,
        'payment_processor' => '5',
        'currency' => 'USD',
        'has_waitlist' => 1,
      );
      $same = FALSE;
      if ($sess = woolman_aval($camp, $i)) {
        $params['id'] = $sess['id'];
        if ((strtotime($sess['start_date']) == strtotime($params['start_date']))
        && (strtotime($sess['end_date']) == strtotime($params['end_date']))
        && ($sess['title'] == $params['title'])
        && ($sess['price'] == $params['price'])
        && ($sess['max_participants'] == $params['max_participants'])) {
          $same = TRUE;
        }
      }
      else {
        $params['created_date'] = date('YmdHis');
        $params['summary'] = 'Another incredible session at ' . $program['event_type'];
        $params['created_id'] = woolman_user_cid();
      }
      if (!$same) {
        $event = woolman_civicrm_api('event', 'create', $params);
        woolman_camp_event_price_create($event['id'], $vals[$type][$s . '_price']);
      }
    }
    if ($vals[$type]['number'] < count($camp)) {
      for ($i = $vals[$type]['number']; $i < count($camp); ++$i) {
        woolman_civicrm_api('event', 'delete', array('id' => $camp[$i]['id']));
      }
    }
  }
  // Save shuttles
  $shuttles = $form_state['storage']['shuttles'][$year];
  foreach (array('coming' => 'Shuttle to Camp (start of session)', 'going' => 'Shuttle from Camp (end of session)') as $dir => $label) {
    $existing = array_values($shuttles[$dir]);
    for ($s = 1; $s <= $vals[$dir]['number']; ++$s) {
      $vals[$dir][$s . '_date']['year'] = $year;
      $params = array(
        'title' => $vals[$dir][$s . '_title'],
        'summary' => $label,
        'description' => $vals[$dir][$s . '_description'],
        'event_type_id' => 10,
        'is_public' => 1,
        'max_participants' => $vals[$dir][$s . '_max'],
        'start_date' => woolman_date_array_to_string($vals[$dir][$s . '_date']).'120000',
        'end_date' => woolman_date_array_to_string($vals[$dir][$s . '_date']).'120000',
        'created_date' => $now,
        'is_active' => 1,
        'currency' => 'USD',
        'has_waitlist' => 1,
        'contribution_type_id' => 5,
        'payment_processor' => '5',
        'created_id' => woolman_user_cid(),
        'custom_40' => $dir,
      );
      $same = FALSE;
      if (!empty($existing[$s-1])) {
        $sess = $existing[$s-1];
        $params['id'] = $sess['event_id'];
        $params['created_date'] = $sess['created_date'];
        if ((strtotime($sess['start_date'])==strtotime($params['start_date']))
          && (strtotime($sess['end_date'])==strtotime($params['end_date']))
          && ($sess['title'] == $params['title'])
          && ($sess['description'] == $params['description'])
          && ($sess['max_participants'] == $params['max_participants'])) {
          $same = TRUE;
        }
      }
      if (!$same) {
        woolman_civicrm_api('event', 'create', $params);
      }
    }
    if ($vals[$dir]['number'] < count($shuttles[$dir])) {
      $existing = array_values($shuttles[$dir]);
      for ($i = $vals[$dir]['number']; $i < count($existing); ++$i) {
        woolman_civicrm_api('event', 'delete', array('id' => $existing[$i]['event_id']));
      }
    }
  }
  unset($form_state['storage']);
  drupal_set_message($year.' dates &amp; rates have been saved');
}
