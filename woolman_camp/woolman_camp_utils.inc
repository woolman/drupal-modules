<?php

civicrm_initialize();

// Event type id of shuttle
define('WOOLMAN_CAMP_SHUTTLE', 10);

/**
 * Returns camp programs (civi event types) managed by this module.
 * TODO: Eventually we will store this in the db and provide a UI
 * WARNING: Until that happens, be very careful to keep the 'name' listed here
 *  consistent with the label in the db for each event type
 */
function woolman_camp_event_types() {
  return array(
    11 => array(
      'name' => 'Mini Camp',
      'age_min' => 6,
      'age_max' => 9,
    ),
    8 => array(
      'name' => 'Camp Woolman',
      'age_min' => 9,
      'age_max' => 14,
    ),
    9 => array(
      'name' => 'Teen Leadership Camp',
      'age_min' => 15,
      'age_max' => 16,
    ),
    12 => array(
      'name' => 'CIT',
      'age_min' => 17,
      'age_max' => 18,
    ),
  );
}

/**
 * Find user's family members
 * Loads most recent camp attendance for each child
 */
function woolman_camp_fetch_relations() {
  static $found = array();
  if ($found) {
    return $found;
  }

  $cid = woolman_user_cid();
  $children = $spouse = $e_contact = $kids = $programs = array();
  $past_reg = FALSE;
  foreach(woolman_camp_event_types() as $id => $program) {
    $programs[$program['name']] = $id;
  }
  if ($relations = woolman_civicrm_api('relationship', 'get', array('contact_id' => $cid), 'values')) {
    //Load contacts and eliminate duplicates
    foreach ($relations as $rel) {
      if ($rel['rtype'] == 'b_a' && $rel['relationship_type_id'] == 1) {
        $children[$rel['cid']] = woolman_contact_get($rel['cid']);
        foreach ($rel as $key => $value) {
          if (substr($key, 0, 8) == 'custom_8')
            $children[$rel['cid']]['relationship_custom'] = $value;
        }
        $events = woolman_civicrm_api('participant', 'get', array('contact_id' => $rel['cid']), 'values');
        $prior = array();
        foreach ($events as $event) {
          if (isset($programs[$event['event_type']])) {
            $prior[$event['event_start_date'] . $event['event_id']] = $event;
          }
        }
        if (!empty($prior)) {
          krsort($prior);
          $past_reg = TRUE;
        }
        $children[$rel['cid']]['prior_camp'] = $prior;
      }
      elseif ($rel['relationship_type_id'] == 2) {
        // If multiple spouses, pick most active relationship
        if ($spouse) {
          if (!$rel['is_active'] || ($spouse['is_active'] && ($spouse['relationship_custom']=='Married' || $spouse['relationship_custom']=='Partners')) || ($rel['start_date'] && $spouse['start_date'] && ($rel['start_date'] < $spouse['start_date'])) || $rel['end_date'])
            continue;
        }
        $spouse = woolman_camp_contact_get($rel['cid']);
        foreach ($rel as $key => $value) {
          if (substr($key, 0, 8) == 'custom_9')
            $spouse['relationship_custom'] = $value;
        }
        if (!$rel['is_active'])
          $spouse['relationship_custom'] = 0;
      }
    }
    if ($children) {
      // Get emergency contact info from kids
      $now = date('Y-m-d');
      $types = array('Emergency Contact is');
      foreach ($children as $child) {
        foreach (woolman_civicrm_api('relationship', 'get', array('contact_id' => $child['contact_id']), 'values') as $contact) {
          if ($contact['is_active'] && $contact['relation'] == 'Emergency Contact is' && (!$contact['end_date'] || $contact['end_date']>$now) && $contact['cid'] != $cid && $contact['cid'] != woolman_aval($spoues, 'contact_id')) {
            $e_contact = woolman_camp_contact_get($contact['cid']);
            break 2;
          }
        }
      }
    }
    if ($spouse) {
      // Load spouse's relation to kids
      foreach (woolman_civicrm_api('relationship', 'get', array('contact_id' => $spouse['contact_id']), 'values') as $rel) {
        if ($rel['rtype'] == 'b_a' && $rel['relationship_type_id'] == 1 && !empty($children[$rel['cid']])) {
          foreach ($rel as $key => $value) {
            if (substr($key, 0, 8) == 'custom_8')
              $children[$rel['cid']]['relationship_to_spouse'] = $value;
          }
        }
      }
    }
    // Sort kids by age
    foreach ($children as $id => $child) {
      $kids[woolman_aval($child, 'birth_date', 0) . $id] = $child;
    }
    krsort($kids);
  }
  $found = array(array_values($kids), $spouse, $e_contact, $past_reg);
  return $found;
}

/**
 * Calculates the age of a camper as of July 1 of a given year
 * If a cid is provided, adjust for any age exception
 */
function woolman_camp_age($dob, $year, $cid = NULL) {
  if ($cid && is_numeric($cid)) {
    $query = db_query("SELECT delta FROM {woolman_camp_age_exception} WHERE year = %d AND cid = %d", $year, $cid);
    if ($exception = db_fetch_object($query)) {
      $year += $exception->delta;
    }
  }
  $camp_date = strtotime($year.'-07-01');
  $birth_date = strtotime($dob);
  return (int)(($camp_date-$birth_date)/60/60/24/365.25);
}

/**
 * Check if a given age is within the limits of a particluar program
 */
function woolman_camp_age_range($age, $program_id, $padding = 0) {
  $programs = woolman_camp_event_types();
  $min = $programs[$program_id]['age_min'] - $padding;
  $max = $programs[$program_id]['age_max'] + $padding;
  return $age >= $min && $age <= $max;
}

/**
 * Minimum and maximum ages for camp (all programs)
 */
function woolman_camp_age_limits($padding = 0) {
  $min = $max = NULL;
  foreach (woolman_camp_event_types() as $event) {
    if ($min === NULL || $min > $event['age_min']) {
      $min = $event['age_min'];
    }
    if ($max === NULL || $max < $event['age_max']) {
      $max = $event['age_max'];
    }
  }
  return array('min' => $min - $padding, 'max' => $max + $padding);
}

/**
 * Gets session dates and attendance numbers
 * Gets campers: by default a simple count, or optionally get details of each camper
 */
function woolman_camp_get_sessions($year, $get_campers = FALSE) {
  $programs = woolman_camp_event_types();
  $event_types = implode(',', array_keys($programs));
  $sessions = array();
  $query = db_query(
    "SELECT ev.id, ev.title, ev.start_date, ev.end_date, ev.event_type_id, ev.max_participants, ps.price_set_id, COUNT(pa.id) AS campers, COUNT(boys.id) AS boys, COUNT(girls.id) AS girls, DATEDIFF(ev.end_date, ev.start_date) as days
    FROM {civicrm_event} ev
    LEFT JOIN {civicrm_price_set_entity} ps ON ps.entity_table = 'civicrm_event' AND ps.entity_id = ev.id
    LEFT JOIN {civicrm_participant} pa ON ev.id = pa.event_id AND pa.status_id IN (SELECT id FROM {civicrm_participant_status_type} WHERE is_counted = 1)
    LEFT JOIN {civicrm_contact} girls ON girls.id = pa.contact_id AND girls.gender_id = 1
    LEFT JOIN {civicrm_contact} boys ON boys.id = pa.contact_id AND boys.gender_id = 2
    WHERE ev.event_type_id IN ($event_types) AND YEAR(ev.start_date) = %d AND ev.is_active = 1 AND ev.is_template <> 1
    GROUP BY ev.id
    ORDER BY ev.event_type_id, ev.start_date
    ", $year);
  while($row = db_fetch_array($query)) {
    $row['full'] = ($row['campers'] >= $row['max_participants']);
    $row['weeks'] = round($row['days'] * 2 / 7) / 2;
    if ($get_campers) {
      $row['campers'] = array();
    }
    $sessions[$row['id']] = $row;
  }
  if ($get_campers && $sessions) {
    $statuses = woolman_civicrm_api('ParticipantStatusType', 'get', array(), 'values');
    $query = db_query(
      "SELECT par.status_id, con.first_name, con.nick_name, con.last_name, con.middle_name, con.gender_id, con.birth_date, par.fee_amount, par.contact_id, par.event_id, par.id AS participant_id, addr.city, YEAR(latest.start_date) AS camper_since, med.date_form_submitted_38 AS medform_date
      FROM {civicrm_participant} par
      INNER JOIN {civicrm_contact} con ON con.id = par.contact_id
      LEFT JOIN {civicrm_value_medical_info_9} med ON med.entity_id = par.contact_id
      LEFT JOIN {civicrm_event} latest ON latest.id =
        ( SELECT op.event_id
          FROM {civicrm_participant} op, {civicrm_event} oe
          WHERE op.contact_id = par.contact_id AND oe.id = op.event_id AND oe.event_type_id IN ($event_types)
          ORDER BY oe.start_date
          LIMIT 1 )
      LEFT JOIN {civicrm_address} addr ON addr.contact_id = par.contact_id AND addr.is_primary = 1
      WHERE par.is_test <> 1 AND con.is_deleted <> 1 AND par.event_id IN (".implode(',', array_keys($sessions)).")");
    while($row = db_fetch_array($query)) {
      $id = $row['event_id'];
      unset($row['event_id']);
      $row['age'] = woolman_camp_age($row['birth_date'], $year);
      $row['status'] = $statuses[$row['status_id']]['name'];
      $row['class'] = $statuses[$row['status_id']]['class'];
      $sessions[$id]['campers'][$row['participant_id'] . '-' . $row['status_id']] = $row;
    }
  }
  return $sessions;
}

/**
 * Sort a flat array of events into a nested array sorted by program
 */
function woolman_camp_sort_sessions($sessions) {
  $programs = woolman_camp_event_types();
  $sorted = array_fill_keys(array_keys($programs), array());
  foreach ($sessions as $id => $session) {
    $sorted[$session['event_type_id']][$id] = $session;
  }
  return array_filter($sorted);
}

/**
 * Gets registration activity from current user
 */
function woolman_camp_get_reg_act() {
  static $act = array();
  if ($act) {
    return $act;
  }
  $query = db_query('
    SELECT a.id, a.status_id
    FROM {civicrm_activity} a, {civicrm_activity_contact} c
    WHERE c.contact_id = %d AND a.activity_type_id = 56 AND a.is_deleted <> 1 AND a.is_test <> 1
    AND a.id = c.activity_id AND c.record_type_id = 2
    AND a.is_current_revision = 1 AND YEAR(a.activity_date_time + INTERVAL 4 month) = ' . woolman_camp_next_year(),
    woolman_user_cid());
  if ($act = db_fetch_array($query)) {
    return $act;
  }
  return array('id' => 0, 'status_id' => 0);
}

/**
 * Updates registration activity status
 * Creates activity if it does not exist
 */
function woolman_camp_update_reg_act($status) {
  $act = woolman_camp_get_reg_act();
  if (!empty($act['id'])) {
    if ($status != $act['status_id']) {
      woolman_civicrm_api('activity', 'create', array(
        'id' => $act['id'],
        'status_id' => $status,
      ));
    }
  }
  else {
    $cid = woolman_user_cid();
    // Create new registration activity
    woolman_civicrm_api('activity', 'create', array(
      'activity_type_id' => 56,
      'source_contact_id' => $cid,
      'target_contact_id' => array($cid),
      'subject' => 'Registration for camp ' . woolman_camp_next_year(),
      'status_id' => $status,
    ));

    // Alert staff of new registration
    $message = array(
      'subject' => 'New camp registration by ' . woolman_name('display'),
      'body' => woolman_name('display') . " just started the registration process.\n\nNote: they will not appear on the registration screen until they complete the form. If they do not do so soon you may wish to contact them:\n" . woolman_contact_url($cid),
    );
    drupal_mail('woolman_website', 0, 'camp@woolman.org', language_default(), $message, 'info@woolman.org');
  }
}

/**
 * Gets camp rates for a given year
 */
function woolman_camp_get_rates($year) {
  static $rates = array();
  if (!isset($rates[$year])) {
    $db = db_query('SELECT * FROM {woolman_camp_rates} WHERE year = %d', $year);
    $rates[$year] = db_fetch_array($db);
    unset($rates[$year]['year']);
  }
  return $rates[$year];
}

/**
 * Retrieve payments for a family
 */
function woolman_camp_get_payments($cid, $year) {
  $payments = array();
  $query = db_query("
    SELECT con.display_name, pay.receive_date, pay.total_amount, pay.check_number, ov.label AS type
    FROM {civicrm_contact} con, {civicrm_contribution} pay
    LEFT JOIN {civicrm_option_value} ov ON ov.option_group_id = 10 AND ov.value = pay.payment_instrument_id
    WHERE con.id = pay.contact_id AND pay.source = %d AND pay.financial_type_id = 5 AND pay.is_test = 0 AND (con.id = %d OR pay.id IN (SELECT sof.contribution_id FROM {civicrm_contribution_soft} sof WHERE sof.contact_id = %d)) AND pay.contribution_status_id = 1
    ORDER BY pay.receive_date", $year, $cid, $cid);
  while($row = db_fetch_array($query)) {
    if ($row['check_number'] && $row['type'] == 'Check') {
      $row['method'] .= 'Check #' . $row['check_number'];
    }
    else {
      $row['method'] = $row['type'];
    }
    // FIXME: Find a better way to compensate for surcharge
    if ($row['type'] == 'Credit Card' && $row['receive_date'] >= 2015) {
      $row['total_amount'] = $row['total_amount'] * (100/103);
    }
    $payments[] = $row;
  }
  return $payments;
}

/**
 * Calculates payments, fees and discounts for a family
 */
function woolman_camp_calc_fees($parent_cid, $year) {
  $programs = woolman_camp_event_types();
  $camp_and_shuttle = array_merge(array(WOOLMAN_CAMP_SHUTTLE), array_keys($programs));
  $rates = woolman_camp_get_rates($year);
  $ret = array(
    'total_fees' => 0,
    'total_discount' => 0,
    'total_paid' => 0,
    'total_approved' => 0,
    'total_pending' => 0,
    'total_waitlist' => 0,
    'weeks' => 0,
    'sessions' => array_fill_keys(array_keys($programs), array('count' => 0, 'weeks' => 0, 'price' => 0)),
    'rates' => $rates,
    'shuttles' => 0,
    'discounts' => array(),
    'campers' => array(),
    'payments' => woolman_camp_get_payments($parent_cid, $year),
  );
  $discount = 0;
  $counted = array();

  // Add up total paid
  foreach ($ret['payments'] as $p) {
    $ret['total_paid'] += $p['total_amount'];
  }

  // Get campers, sessions & shuttles
  $db = db_query('
    SELECT st.name AS status, st.class, st.is_counted, con.first_name, con.nick_name, con.last_name, con.gender_id, ev.id AS event_id, ev.title, pa.contact_id, ev.start_date, ev.end_date, ev.event_type_id, DATEDIFF(ev.end_date, ev.start_date) AS days, pa.register_date, pa.fee_amount, pa.id AS pid, med.date_form_submitted_38 as medform_date
    FROM {civicrm_event} ev
    INNER JOIN {civicrm_participant} pa ON ev.id = pa.event_id AND pa.status_id IN (SELECT id FROM {civicrm_participant_status_type} WHERE class <> \'Negative\')
    INNER JOIN {civicrm_contact} con ON con.id = pa.contact_id
    INNER JOIN {civicrm_participant_status_type} st ON st.id = pa.status_id
    LEFT JOIN {civicrm_value_medical_info_9} med ON med.entity_id = pa.contact_id
    WHERE con.is_deleted <> 1 AND ev.event_type_id IN (' . implode(',', $camp_and_shuttle) . ') AND YEAR(ev.start_date) = %d AND ev.is_active = 1 AND ev.is_template <> 1 AND pa.is_test <> 1 AND (pa.contact_id IN (SELECT contact_id FROM {civicrm_participant} WHERE id IN (SELECT entity_id FROM {civicrm_value_registration_questions_7} WHERE parent_15 = %d)) OR pa.contact_id = %d)
    ORDER BY con.birth_date DESC', $year, $parent_cid, $parent_cid);
  while ($row = db_fetch_array($db)) {
    if (!$row['event_id'] || !$row['contact_id']) {
      continue;
    }
    $cid = $row['contact_id'];
    if (empty($ret['campers'][$cid])) {
      $ret['campers'][$cid] = array(
        'register_date' => NULL,
        'weeks' => 0,
        'sessions' => array(),
        'session_names' => '',
      );
    }
    //Shuttle
    if ($row['event_type_id'] == WOOLMAN_CAMP_SHUTTLE) {
      if ($row['is_counted'] && !isset($ret['campers'][$cid]['shuttle'][$row['event_id']])) {
        ++$ret['shuttles'];
        $ret['total_fees'] += $rates['shuttle_price'];
        if ($cid != $parent_cid) {
          $ret['campers'][$cid]['shuttle'][$row['pid']] = array(
            'title' => $row['title'],
            'start_date' => $row['start_date'],
            'event_id' => $row['event_id'],
            'pid' => $row['pid'],
            'fee_amount' => $row['fee_amount'],
            'register_date' => $row['register_date'],
          );
        }
      }
      continue;
    }
    if (empty($register_date) || $row['register_date'] < $register_date) {
      $register_date = $row['register_date'];
    }
    if (empty($ret['campers'][$cid]['register_date']) || ($row['register_date'] && $row['register_date'] < $ret['campers'][$cid]['register_date'])) {
      $ret['campers'][$cid]['register_date'] = $row['register_date'];
    }
    $row['weeks'] = $weeks = round($row['days'] * 2 / 7) / 2;
    $ret['weeks'] += $weeks;
    $row['price'] = woolman_camp_event_price_get(array('id' => $row['event_id']));
    $ret['sessions'][$row['event_type_id']]['weeks'] += $weeks;
    $ret['sessions'][$row['event_type_id']]['count']++;
    $ret['sessions'][$row['event_type_id']]['price'] += $row['price'];
    $ret['total_fees'] += $row['price'];
    $ret['campers'][$cid]['weeks'] += $weeks;
    $ret['campers'][$cid]['first_name'] = $row['nick_name'] ? $row['nick_name'] : $row['first_name'];
    $ret['campers'][$cid]['last_name'] = $row['last_name'];
    $ret['campers'][$cid]['gender_id'] = $row['gender_id'];
    $ret['campers'][$cid]['medform_date'] = $row['medform_date'];
    $ret['campers'][$cid]['total_paid'] += $row['fee_amount'];
    if ($row['is_counted'] && (int) $weeks) {
      $counted[$cid] = $cid;
    }
    if ($ret['campers'][$cid]['session_names']) {
      $ret['campers'][$cid]['session_names'] .= ', ';
    }
    $ret['campers'][$cid]['session_names'] .= trim(str_replace($year, '', $row['title']));
    if ($row['status'] == 'Pending from pay later' || $row['status'] == 'Registered') {
      $ret['total_approved']++;
      $row['message'] = $row['fee_amount'] >= ($rates['deposit'] * $row['weeks']) ? 'Registered.' : 'Registration approved. Please pay deposit to reserve your space in this session.';
    }
    elseif ($row['status'] == 'Awaiting approval') {
      $ret['total_pending']++;
      $row['message'] = 'Our staff are reviewing your application and will contact you soon.';
    }
    elseif ($row['status'] == 'On waitlist') {
      $ret['total_waitlist']++;
      $row['message'] = 'On waitlist. We will contact you as soon as space becomes available.';
    }
    unset($row['nick_name'], $row['first_name'], $row['last_name'], $row['contact_id'], $row['gender_id'], $row['status_id'], $row['medform_date']);
    $ret['campers'][$cid]['sessions'][$row['pid']] = $row;
  }
  // Calculate discounts
  foreach ($ret['campers'] as $camper) {
    // Early registration discount
    if (!empty($rates['early_discount']) && (int) $camper['weeks'] && (substr($rates['early_date'], 0, 10) >= substr($camper['register_date'], 0, 10))) {
      $ret['total_discount'] += $rates['early_discount'];
      if (empty($ret['discounts']['early_discount'])) {
        $ret['discounts']['early_discount'] = array(
          'date' => $register_date,
          'amount' => 0,
          'type' => 'Early Registration',
          'id' => NULL,
        );
      }
      $ret['discounts']['early_discount']['amount'] += $rates['early_discount'];
    }
  }
  // Multi-week discount - special case for camp woolman (id 8)
  $weeks = $ret['sessions'][8]['weeks'];
  $multi_week_discount = $rates['sfc_multi_week_discount'] * $weeks;
  if ($weeks > 1 && $multi_week_discount) {
    $ret['total_discount'] += $multi_week_discount;
    $ret['discounts']['sfc_multi_week_discount'] = array(
      'date' => $register_date,
      'amount' => $multi_week_discount,
      'type' => 'Multi-Week Discount',
      'id' => NULL,
    );
  }
  // Family discount -- check if there's more than one child registered
  if (!empty($rates['family_discount']) && count($counted) > 1) {
    $ret['total_discount'] += $rates['family_discount'] * count($counted);
    $ret['discounts']['family_discount'] = array(
      'amount' => $rates['family_discount'] * count($counted),
      'date' => $register_date,
      'type' => 'Family Discount',
      'id' => NULL,
    );
  }
  // Other discounts
  $db = db_query('SELECT * FROM {woolman_camp_discounts} WHERE cid = %d AND year = %d', $parent_cid, $year);
  while ($row = db_fetch_array($db)) {
    unset($row['cid'], $row['year']);
    $ret['total_discount'] += $row['amount'];
    $ret['discounts'][$row['id']] = $row;
  }
  $ret['shuttle_price'] = $ret['shuttles'] * woolman_aval($rates, 'shuttle_price', 0);
  $ret['register_date'] = $register_date;
  $ret['deposit'] = woolman_aval($rates, 'deposit', 0) * $ret['weeks'];
  return $ret;
}

/**
 * Format session dates
 * @param str: start date
 * @param str: end date
 * @return Weeks, Date(s)
 */
function woolman_camp_date_format($start, $end = NULL, $return_length = TRUE) {
  $start = strtotime($start);
  if ($end) {
    $end = strtotime($end);
    if (date('F', $start) == date('F', $end))
      $date = date('F j', $start) . ' - ' . date('jS', $end);
    else
      $date = date('F j', $start) . ' - ' . date('F j', $end);
    $dates = str_replace('August', 'Aug', $date);
    if (!$return_length) {
      return $dates;
    }
    $length = round(($end - $start) / 60 / 60 / 24 / 7 * 2) / 2;
    $unit = 'week';
    if ($length < 1) {
      $length = round(($end - $start) / 60 / 60 / 24);
      $unit = 'day';
    }
    return "<strong>$length $unit" . ($length == 1 ? '' : 's') . ':</strong> ' . $dates;
  }
  else {
    return date('F jS', $start);
  }
}

/**
 * Loads a contact and parses dob & location info
 */
function woolman_camp_contact_get($cid = NULL) {

  $contact = woolman_contact_get($cid, NULL, TRUE);

  if (!empty($contact['birth_date'])) {
    $contact['dob'] = woolman_date_string_to_array($contact['birth_date']);
  }

  $phones = $emails = array('home' => '', 'work' => '', 'cell' => '', 'primary' => '');
  foreach ($contact['locations']['phone'] as $phone) {
    if ($phone['location_type_id'] == 4 && $phones['primary'] != 'cell') {
      $phones['cell'] = $phone['phone'];
      if ($phone['is_primary']) $phones['primary'] = 'cell';
    }
    if ($phone['location_type_id'] == 1 && $phones['primary'] != 'home') {
      $phones['home'] = $phone['phone'];
      if ($phone['is_primary']) $phones['primary'] = 'home';
    }
    if ($phone['location_type_id'] == 2 && $phones['primary'] != 'work') {
      $phones['work'] = $phone['phone'];
      if ($phone['is_primary']) $phones['primary'] = 'work';
    }
  }
  $contact['phones'] = $phones;

  foreach ($contact['locations']['email'] as $email) {
    if ($email['location_type_id'] == 1 && $emails['primary'] != 'home') {
      $emails['home'] = $email['email'];
      if ($email['is_primary']) {
        $emails['primary'] = 'home';
      }
    }
    if ($email['location_type_id'] == 2 && $emails['primary'] != 'work') {
      $emails['work'] = $email['email'];
      if ($email['is_primary']) {
        $emails['primary'] = 'work';
      }
    }
  }
  $contact['emails'] = $emails;

  return $contact;
}

/**
 * Returns selectable discount types
 */
function woolman_camp_discount_types() {
  return drupal_map_assoc(array(
    'Financial Aid',
    'Discount Voucher',
    'Work-Trade',
    'Tell A Friend Discount',
    'Staff Discount',
    'Family Discount',
    'Account Credit',
  ));
}

/**
 * Fetch the price of an event
 *
 * @param array: $event
 */
function woolman_camp_event_price_get($event) {
  $field = woolman_camp_event_price_field_get($event);
  if ($field) {
    $price = woolman_civicrm_api('price_field_value', 'get', array('price_field_id' => $field['id'], 'is_default' => 1), 'values');
    foreach ($price as $value) {
      if (!empty($value['is_default'])) {
        return $value['amount'];
      }
    }
  }
}

/**
 * Save the price of an event
 *
 * @param int: $event_id
 * @param int: $price
 */
function woolman_camp_event_price_create($event_id, $price) {
  $params = array('amount' => $price);
  $params['name'] = $params['label'] = 'Camp Fee';
  $params['is_default'] = $params['is_active'] = 1;

  $event_params = array('id' => $event_id);
  $field = woolman_camp_event_price_field_get($event_params);

  // If field/set doesn't exist, create them
  if (!$field) {
    $event = woolman_civicrm_api('event', 'get', $event_params, 'single');
    $set = woolman_civicrm_api('price_set', 'create', array(
      'entity_id' => $event_id,
      'entity_table' => 'civicrm_event',
      'name' => $event['title'],
      'title' => $event['title'],
      'is_active' => 1,
      'extends' => 1,
      'is_quick_config' => 1,
    ));
    $field = woolman_civicrm_api('price_field', 'create', array(
      'price_set_id' => $set['id'],
      'name' => 'Camp Fee',
      'label' => 'Camp Fee',
      'html_type' => 'Radio',
      'is_active' => 1,
    ));
  }
  else {
    $price = woolman_civicrm_api('price_field_value', 'get', array('price_field_id' => $field['id'], 'is_default' => 1), 'values');
    foreach ($price as $value) {
      if (!empty($value['is_default'])) {
        $params['id'] = $value['id'];
      }
    }
  }
  $params['price_field_id'] = $field['id'];
  woolman_civicrm_api('price_field_value', 'create', $params);
}

/**
 * Fetch the price field of an event
 *
 * @param array : $event
 * @return array
 */
function woolman_camp_event_price_field_get($event) {
  if (empty($event['price_set_id'])) {
    $event['price_set_id'] = CRM_Core_DAO::singleValueQuery("SELECT price_set_id
      FROM civicrm_price_set_entity
      WHERE entity_table = 'civicrm_event' AND entity_id = {$event['id']} LIMIT 1"
    );
  }
  if ($event['price_set_id']) {
    return woolman_civicrm_api('price_field', 'get', array('price_set_id' => $event['price_set_id']), 'single');
  }
}

/**
 * Get shuttle dates, info & attendance
 */
function woolman_camp_get_shuttle($year, $get_passengers) {
  $select = 'ev.id AS event_id, ev.title, ev.description, ev.max_participants, ev.start_date, sh.direction_40 AS direction';
  $from = '{civicrm_event} ev LEFT JOIN {civicrm_value_shuttle_info_11} sh ON sh.entity_id = ev.id';
  if ($get_passengers) {
    $select .= ', con.sort_name, con.display_name, con.id AS contact_id, pa.id AS participant_id';
    $from .= '
      LEFT JOIN {civicrm_participant} pa ON ev.id = pa.event_id AND pa.status_id IN (SELECT id FROM {civicrm_participant_status_type} WHERE is_counted) AND pa.is_test <> 1
      LEFT JOIN {civicrm_contact} con ON con.is_deleted <> 1 AND con.id = pa.contact_id';
  }
  $db = db_query("SELECT $select FROM $from
    WHERE ev.event_type_id = " . WOOLMAN_CAMP_SHUTTLE . " AND YEAR(ev.start_date) = %d
      AND ev.is_active <> 0 AND ev.is_template <> 1
    ORDER BY ev.start_date", $year);
  $ret = array('coming' => array(), 'going' => array());
  while ($row = db_fetch_array($db)) {
    if ($eid = $row['event_id']) {
      $dir = $row['direction'];
      $ret[$dir][$eid]['title'] = $row['title'];
      $ret[$dir][$eid]['description'] = $row['description'];
      $ret[$dir][$eid]['max_participants'] = $row['max_participants'];
      $ret[$dir][$eid]['start_date'] = $row['start_date'];
      $ret[$dir][$eid]['direction'] = $dir;
      $ret[$dir][$eid]['event_id'] = $eid;
      if ($get_passengers) {
        if (!isset($ret[$dir][$eid]['campers'])) {
          $ret[$dir][$eid]['campers'] = array();
        }
        if ($cid = $row['contact_id']) {
          $ret[$dir][$eid]['campers'][$cid] = array(
            'display_name' => $row['display_name'],
            'sort_name' => $row['sort_name'],
            'participant_id' => $row['participant_id'],
          );
        }
      }
    }
  }
  return $ret;
}

/**
 * Update event fees to be in sync with actual payments
 * TODO: Has not been tested to work with negative amount (i.e. failed transaction or refund)
 */
function woolman_camp_update_totals($reg, $amount) {
  // Distribute the payment among all events
  $rates = $reg['rates'];
  $owed = $reg['total_fees'] - $reg['total_discount'] - $reg['total_paid'];
  $payout = $amount;
  if (isset($reg['discounts']['sfc_multi_week_discount'])) {
    $reg['total_discount'] -= $reg['discounts']['sfc_multi_week_discount']['amount'];
  }
  $discount = $reg['total_discount'] / $reg['weeks'];
  // First do the camp sessions
  foreach ($reg['campers'] as $kid) {
    foreach ($kid['sessions'] as $sess) {
      $price = $sess['price'];
      if (isset($reg['discounts']['sfc_multi_week_discount']) && $sess['event_type_id'] == 8) {
        $price -= $rates['sfc_multi_week_discount'] * $sess['weeks'];
      }
      if ($sess['fee_amount'] < $price) {
        if ($amount >= $owed - $reg['shuttle_price']) {
          $new_amount = $price;
          $payout -= $price - $sess['fee_amount'];
        }
        else {
          $portion = ($amount / $reg['weeks']) * $sess['weeks'];
          $new_amount = $sess['fee_amount'] + $portion;
          $payout -= $portion;
        }
        CRM_Core_DAO::executeQuery("UPDATE civicrm_participant SET fee_amount = $new_amount WHERE id = " . $sess['pid']);
      }
    }
  }
  // Then the shuttles
  foreach ($reg['campers'] as $kid) {
    foreach ($kid['shuttle'] as $shut) {
      $owe = $rates['shuttle_price'] - $shut['fee_amount'];
      if ($payout > 0 && $owe) {
        if ($payout < $owe) {
          $portion = $payout;
          $new_amount = $shut['fee_amount'] + $portion;
        }
        else {
          $portion = $owe;
          $new_amount = $rates['shuttle_price'];
        }
        $payout -= $portion;
        CRM_Core_DAO::executeQuery("UPDATE civicrm_participant SET fee_amount = $new_amount WHERE id = " . $shut['pid']);
      }
    }
  }
}

/**
 * Search for new camp payments that haven't been processed and, well... process them.
 * Called via cron as well as via hook_civicrm_post.
 * Cron was necessary because CiviCRM hooks do not fire for recurring contributions.
 */
function woolman_camp_process_payments($cron) {
  $db = CRM_Core_DAO::executeQuery("SELECT c.id, c.contact_id, s.contact_id AS soft_credit_to, c.contribution_page_id, c.total_amount
    FROM civicrm_contribution c
    LEFT JOIN civicrm_contribution_soft s ON c.id = s.contribution_id
    WHERE c.financial_type_id = 5 AND c.contribution_status_id = 1 AND (c.source IS NULL OR c.source NOT LIKE '20%')");
  while ($db->fetch()) {
    $year = woolman_camp_next_year();
    $cid = empty($db->soft_credit_to) ? $db->contact_id : $db->soft_credit_to;
    if ($reg = woolman_camp_calc_fees($cid, $year)) {
      if (!$cron && $db->contribution_page_id == 7 && woolman_user_cid() == $db->contact_id) {
        drupal_set_message('Thank you for your payment. ' . l('Click here to return to your camp registration page.', '~camp/register/status', array('alias' => TRUE)));
      }
      woolman_camp_update_totals($reg, $db->total_amount);
    }
    else {
      // Couldn't figure out what to do with this payment - need manual intervention
      $message = array(
        'subject' => 'Unprocessed Camp Payment - Please Review',
        'body' => "A payment of \${$db->total_amount} was recieved from " . woolman_name('display', $db->contact_id) .
        ".\nHowever this person does not have any registered campers. Please edit this payment and give soft-credit to the person who registered for camp.\n\n" . woolman_contact_url($db->contact_id, array('op' => 'contribution', 'query' => array('action' => 'update', 'id' => $db->id))),
      );
      drupal_mail('woolman_website', 0, 'camp@woolman.org', language_default(), $message, 'info@woolman.org');
      $year = '20??';
    }
    // Set contribution source
    CRM_Core_DAO::executeQuery("UPDATE civicrm_contribution SET source = '$year' WHERE id = " . $db->id);
  }
}
