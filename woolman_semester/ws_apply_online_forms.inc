<?php
/**
 * Forms and form validation functions
 * Part of the WS Online Application module
 */

module_load_include('inc', 'woolman_semester', 'ws_apply_online_form_submit');

/**
 * Get Started Button
 */
function _ws_apply_form_get_started($form_state) {
  //build form
  $form = array(
    '#prefix' => '<strong>Your Name:</strong> <span class="yourname">' . woolman_name('full') . '</span> <span class="description">(Not you? Please log in as yourself before beginning your application.)</span>',
    'semester_choices' => array(
      '#type' => 'checkboxes',
      '#description' => 'Which semester are you applying for? (you can check more than one if you haven\'t decided yet)',
      '#prefix' => '<div class="subset">',
      '#suffix' => '</div>',
      '#required' => TRUE,
      '#options' => woolman_upcoming_semesters(4),
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => 'Begin Application',
      '#prefix' => '<div style="clear:both;">',
      '#suffix' => '</div>',
    ),
  );
  return $form;
}

/**
 * Basic Application Form
 */
function _ws_apply_form_type_form($form_state) {
  drupal_add_js(drupal_get_path('module', 'woolman_semester') . '/js/ws_apply_form_basics.js', 'module', 'header', TRUE, FALSE);

  //get info about applicant
  $self = woolman_contact_get(NULL, array(59));
  $parent = $sorted = array();
  $self['dob'] = empty($self['birth_date']) ? NULL : woolman_date_string_to_array($self['birth_date']);

  //fetch parents
  if ($relations = woolman_find_relations($self['contact_id'], 1)) {
    //Load contacts
    foreach ($relations as $rel) {
      if ($rel['relationship_name'] == 'Child of') {
        $sorted[$rel['contact_id']] = woolman_contact_get($rel['contact_id'], array('job_title'));
      }
    }
    //Put parents in same household as student first
    $count = 0;
    foreach ($sorted as $rel) {
      if (!empty($rel['street_address']) && $rel['street_address'] == woolman_aval($self, 'street_address')) {
        $parent[++$count] = $rel;
      }
    }
    foreach ($sorted as $rel) {
      if (empty($rel['street_address']) || $rel['street_address'] != woolman_aval($self, 'street_address')) {
        $parent[++$count] = $rel;
      }
    }
  }
  //get options
  $parent_roles = woolman_get_civi_options(83);

  //get phones
  $person = &$self;
  for ($i=0; $i<=2; ++$i) {
    if ($i) {
      if (empty($parent[$i])) {
        break;
      }
      $person = &$parent[$i];
    }
    $result = woolman_civicrm_api('phone', 'get', array('contact_id' => $person['contact_id']));
    foreach($result['values'] as $p) {
      switch ($p['location_type_id']) {
        case 1:
          $person['home_phone'] = $p['phone'];
          break;
        case 2:
          $person['work_phone'] = $p['phone'];
          break;
        case 4:
          $person['cell_phone'] = $p['phone'];
          break;
      }
    }
  }

  //build form
  $form = array();
  $form['#prefix'] = '<p>' . _ws_apply_settings('ws_apply_basic_form_instructions', TRUE) . '</p>';

  $form['self'] = array(
    '#type' => 'fieldset',
    '#title' => 'About Yourself',
  );
    $form['self']['self_first'] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#title' => 'First name',
      '#description' => 'Your real first name',
      '#default_value' => woolman_aval($self, 'first_name'),
      '#size' => 15,
      '#req' => 1,
    );
    $form['self']['self_nick'] = array(
      '#type' => 'textfield',
      '#title' => 'Nickname',
      '#description' => 'The name you go by',
      '#default_value' => woolman_aval($self, 'nick_name') == woolman_aval($self, 'first_name') ? '' : $self['nick_name'],
      '#size' => 15,
    );
    $form['self']['self_last'] = array(
      '#type' => 'textfield',
      '#title' => 'Last name',
      '#default_value' => woolman_aval($self, 'last_name'),
      '#size' => 20,
      '#req' => 1,
    );
    $form['self']['self_dob'] = array(
      '#type' => 'date',
      '#title' => 'Birthdate',
      '#default_value' => woolman_aval($self, 'dob', array('year' => 1900,'month' => 1,'day' => 1)),
      '#req' => 1,
      '#suffix' => '</div>',
    );
    $form['self']['self_home_phone'] = array(
      '#type' => 'textfield',
      '#prefix' => '<div class="subset">',
      '#title' => 'Home Phone',
      '#size' => 15,
      '#default_value' => woolman_aval($self, 'home_phone'),
    );
    $form['self']['self_cell_phone'] = array(
      '#type' => 'textfield',
      '#title' => 'Cell Phone',
      '#size' => 15,
      '#default_value' => woolman_aval($self, 'cell_phone'),
    );

    $form['self']['self_email'] = array(
      '#type' => 'textfield',
      '#title' => 'Email Address',
      '#default_value' => woolman_aval($self, 'email'),
      '#size' => 25,
      '#req' => 1,
    );
    $form['self']['self_gender'] = array(
      '#type' => 'radios',
      '#title' => 'Gender',
      '#default_value' => woolman_aval($self, 'gender_id'),
      '#options' => array(1 => 'Female', 2 => 'Male'),
      '#req' => 1,
      '#suffix' => '</div>',
    );
    $form['self']['self_ethnicity'] = array(
      '#type' => 'checkboxes',
      '#title' => 'Ethnicity',
      '#default_value' => woolman_multivalue_arr(woolman_aval($self, 'custom_59')),
      '#options' => woolman_get_civi_options(118),
      '#description' => 'Check all that apply. (This optional information is kept confidential and has no bearing on your application.)',
      '#req' => 1,
      '#prefix' => '<div class="subset">',
      '#suffix' => '</div>',
    );
    $form['self']['self_street'] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#title' => 'Street Address',
      '#default_value' => woolman_aval($self, 'street_address'),
      '#size' => 25,
      '#req' => 1,
    );
    $form['self']['self_city'] = array(
      '#type' => 'textfield',
      '#title' => 'City',
      '#default_value' => woolman_aval($self, 'city'),
      '#size' => 10,
      '#req' => 1,
    );
    $form['self']['self_state'] = array(
      '#type' => 'textfield',
      '#title' => 'State/Provence',
      '#default_value' => woolman_aval($self, 'state_province'),
      '#size' => 3,
      '#maxlength' => 2,
      '#req' => 1,
    );
    $form['self']['self_zip'] = array(
      '#type' => 'textfield',
      '#title' => 'Zip Code',
      '#default_value' => woolman_aval($self, 'postal_code'),
      '#size' => 8,
      '#req' => 1,
    );
    $form['self']['self_country'] = array(
      '#type' => 'select',
      '#title' => 'Country',
      '#options' => $countries = woolman_get_civi_options('country'),
      '#default_value' => woolman_aval($self, 'country_id', USA),
      '#req' => 1,
      '#suffix' => '</div>',
    );

    $form['self']['self_medium'] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'select',
      '#title' => 'How did you hear about Woolman?',
      '#description' => 'Choose an option',
      '#options' => woolman_get_civi_options(82),
    );
    $form['self']['self_details'] = array(
      '#type' => 'textfield',
      '#title' => 'Details',
      '#description' => 'Please describe how you heard about us',
      '#req' => 1,
      '#suffix' => '</div>',

    );

  //Build fieldset for each parent
  for ($i=1; $i<=2; $i++) {
    $parent[$i]['dob'] = NULL;
    if (!empty($parent[$i]['birth_date'])) {
      $parent[$i]['dob'] = woolman_date_string_to_array($parent[$i]['birth_date']);
    }
    $form['parent' . $i] = array(
      '#type' => 'fieldset',
      '#title' => 'Parent/Guardian ' . $i,
      '#attributes' => array('class' => 'two-up num-' . $i),
      '#description' => '<p>'.($i==1 ? 'Primary caregiver. This person will be sent our parent/guardian questionnaire' : 'Enter a second caregiver if applicable').'.</p>',
    );
    if ($i == 1) {
      $form['parent' . $i]['#prefix'] = '<div style="clear:both">';
    }
    else {
      $form['parent' . $i]['#suffix'] = '</div>';
    }
    $form['parent' . $i]['parent' . $i . '_first'] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#title' => 'First name',
      '#default_value' => woolman_aval($parent, "$i:first_name"),
      '#size' => 15,
      '#req' => ($i==1),
    );
    $form['parent' . $i]['parent' . $i . '_nick'] = array(
      '#type' => 'textfield',
      '#title' => 'Nickname',
      '#default_value' => woolman_aval($parent, "$i:nick_name") == woolman_aval($parent, "$i:first_name") ? '' : $parent[$i]['nick_name'],
      '#size' => 12,
    );
    $form['parent' . $i]['parent' . $i . '_last'] = array(
      '#type' => 'textfield',
      '#title' => 'Last name',
      '#default_value' => woolman_aval($parent, "$i:last_name"),
      '#size' => 20,
      '#req' => ($i==1),
      '#suffix' => '</div>',
    );
    $form['parent' . $i]['parent' . $i . '_relationship'] = array(
      '#type' => 'select',
      '#title' => 'Relationship to you',
      '#options' => array('' => '- Select -') + $parent_roles,
      '#req' => ($i==1),
      '#prefix' => '<div class="subset">',
    );
    if ($i==2) {
      $form['parent2']['parents_relationship'] = array(
        '#type' => 'select',
        '#title' => 'Relationship to <span class="p1-name">Primary Caregiver</span>',
        '#options' => array(0=> 'Other/None', 2=> 'Married', 'divorced' => 'Divorced', 16=> 'Family Relative'),
        '#default_value' => 2,
      );
    }
    $form['parent' . $i]['parent' . $i . '_employer'] = array(
      '#type' => 'textfield',
      '#title' => 'Job Title &amp; Employer',
      '#default_value' => woolman_aval($parent, "$i:job_title"),
      '#size' => 20,
      '#suffix' => '</div>',
    );
    $form['parent' . $i]['parent' . $i . '_gender'] = array(
      '#type' => 'radios',
      '#title' => 'Gender',
      '#default_value' => woolman_aval($parent, "$i:gender_id"),
      '#options' => array(1=> 'Female', 2=> 'Male'),
      '#req' => ($i==1),
      '#prefix' => '<div class="subset">',
    );
      $form['parent' . $i]['parent' . $i . '_dob'] = array(
      '#type' => 'date',
      '#title' => 'Birthdate',
      '#default_value' => woolman_aval($parent[$i], 'dob', array('year' => 1900,'month' => 1,'day' => 1)),
      '#req' => ($i==1),
      '#suffix' => '</div>',
    );
    $form['parent' . $i]['parent' . $i . '_home_phone'] = array(
      '#type' => 'textfield',
      '#prefix' => '<div class="subset">',
      '#title' => 'Home Phone',
      '#size' => 12,
      '#default_value' => woolman_aval($parent[$i], 'home_phone'),
    );
    $form['parent' . $i]['parent' . $i . '_work_phone'] = array(
      '#type' => 'textfield',
      '#title' => 'Work Phone',
      '#size' => 12,
      '#default_value' => woolman_aval($parent[$i], 'work_phone'),
    );
    $form['parent' . $i]['parent' . $i . '_cell_phone'] = array(
      '#type' => 'textfield',
      '#title' => 'Cell Phone',
      '#size' => 12,
      '#default_value' => woolman_aval($parent[$i], 'cell_phone'),
      '#suffix' => '</div>',
    );
    $form['parent' . $i]['parent' . $i . '_email'] = array(
      '#type' => 'textfield',
      '#title' => 'Email Address',
      '#default_value' => woolman_aval($parent, "$i:email"),
      '#size' => 20,
      '#req' => ($i==1),
      '#prefix' => '<div class="subset">',
    );
    $form['parent' . $i]['parent' . $i . '_email_test'] = array(
      '#type' => 'textfield',
      '#title' => 'Retype Email',
      '#default_value' => woolman_aval($parent, "$i:email"),
      '#size' => 20,
      '#req' => ($i==1),
      '#suffix' => '</div>',
    );
    $form['parent' . $i]['parent' . $i . '_same'] = array(
      '#type' => 'checkbox',
      '#title' => 'Same as my address',
      '#prefix' => '<div class="fieldset" style="min-height:3.6em"><div id="sameaddress' . $i . '" style="float:right">',
      '#suffix' => '</div>',
      '#default_value' => (!empty($self['street_address']) && $self['street_address'] == woolman_aval($parent, "$i:street_address")),
    );
    $form['parent' . $i]['parent' . $i . '_street'] = array(
      '#type' => 'textfield',
      '#title' => 'Street Address',
      '#default_value' => woolman_aval($parent, "$i:street_address"),
      '#size' => 30,
      '#attributes' => array('style' => 'margin-top:0'),
    );
    $form['parent' . $i]['parent' . $i . '_city'] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#title' => 'City',
      '#default_value' => woolman_aval($parent, "$i:city"),
      '#size' => 10,
    );
    $form['parent' . $i]['parent' . $i . '_state'] = array(
      '#type' => 'textfield',
      '#title' => 'St/Prov',
      '#default_value' => woolman_aval($parent, "$i:state_province"),
      '#size' => 3,
      '#maxlength' => 2,
    );
    $form['parent' . $i]['parent' . $i . '_zip'] = array(
      '#type' => 'textfield',
      '#title' => 'Zip/Postal',
      '#default_value' => woolman_aval($parent, "$i:postal_code"),
      '#size' => 8,
      '#suffix' => '</div>',
    );
    $form['parent' . $i]['parent' . $i . '_country'] = array(
      '#type' => 'select',
      '#title' => 'Country',
      '#options' => $countries,
      '#default_value' => woolman_aval($parent, "$i:country_id", USA),
      '#suffix' => '</div>',
    );
  }
  _ws_apply_add_buttons($form, 'basic');
  $form['buttons']['submit']['#validate'][] = '_ws_apply_form_basic_validate';
  return _ws_apply_read_draft($form, 35);
}


/**
 * School Form
 * Also handles AHAH requests for building lists of schools on-the-fly
 */
function _ws_apply_form_type_school($form_state) {
  drupal_add_js(drupal_get_path('module', 'woolman_semester') .'/js/ws_apply_form_school.js', 'module', 'header', TRUE, FALSE);
  drupal_add_css(drupal_get_path('module', 'woolman_semester') . '/css/ws_apply_online.css');
  drupal_set_title('School Information');

  $student = woolman_contact_get(NULL, array(4));
  $case = _ws_apply_get_case($student['contact_id']);
  $type_default = NULL;
  $grad_default = array('month' => 6, 'year' => 2000, 'day' => 1);

  //get saved values or saved draft
  if (!($saved = woolman_aval($form_state, 'values', array()))) {
    if (!($id = $case['acts'][42][0]['original_id'])) {
      $id = $case['acts'][42][0]['id'];
    }
    $query = db_query('SELECT content FROM {ws_apply_online_drafts} WHERE id = %d', $id);
    if ($serial = db_fetch_array($query)) {
      $saved = unserialize($serial['content']);
    }
  }

  // if form is loading for the 1st time, set some defaults
  if (!$saved) {
    //fetch relationship info
    if ($relations = woolman_find_relations($student['contact_id'], 10, FALSE)) {
      foreach ($relations as $rel) {
        $result = woolman_civicrm_api('contact', 'get', array('contact_id' => $rel['contact_id']), 'single');
        if (woolman_aval($result, 'contact_sub_type') == 'High_School' && !empty($result['country_id']) && !empty($result['state_province_id'])) {
          $school = $result;
          if (empty($rel['is_active']) || ($rel['end_date'] && strtotime($rel['end_date']) < time())) {
            $type_default = 'grad';
          }
          break;
        }
      }
    }

    if (!empty($student['custom_4'])) {
      $grad_date = explode('-', $student['custom_4']);
      $grad_default = array('month' => intval($grad_date[1]), 'year' => intval($grad_date[0]));
      if ($grad_date[0] . $grade_date[1] >= date('Ym')) {
        $type_default = 'grad';
      }
    }
  }

  //get countries and states
  $states_countries = array();
  $countries = woolman_get_civi_options('country');
  $countries_with_states = array(USA => 'USA'); //enter id and abbr of all countries that should have states listed
  $resource = db_query('  SELECT id, name AS state, country_id
                          FROM {civicrm_state_province}
                          WHERE country_id IN (' . implode(',', array_keys($countries_with_states)) . ')
                          ORDER BY name');
  while ($st = db_fetch_array($resource)) {
    $states[$st['country_id']][$st['country_id'].'-' . $st['id']] = woolman_aval($countries_with_states, $st['country_id']).': ' . $st['state'];
  }
  foreach ($countries as $id=> $con) {
    if (array_key_exists($id, $states)) {
      $states_countries = array_merge($states_countries, $states[$id]);
    }
    else {
      $states_countries["$id-0"] = $con;
    }
  }

  //find schools
  if (!($location = woolman_aval($saved, 'school_location'))) {
    $var = ($school ? 'school' : 'student');
    if (array_key_exists(${$var}['country_id'], $countries_with_states)) {
      $location = ${$var}['country_id'] . '-' . ${$var}['state_province_id'];
    }
    else {
      $location = ${$var}['country_id'] . '-0';
    }
  }

  $params = array(
    'contact_type' => 'Organization',
    'contact_sub_type' => 'High_School',
    'sort' => 'organization_name ASC',
    'return.organization_name' => 1,
    'return.city' => 1,
    'return.state_province' => 1,
    'return.country' => 1,
  );
  list($sch_country, $sch_state) = explode('-', $location);
  if ($sch_country) {
    $params['country'] = $sch_country;
  }
  if ($sch_state) {
    $params['state_province'] = $sch_state;
  }
  if ($school_search = woolman_civicrm_api('contact', 'get', $params, 'values')) {
    foreach ($school_search as $id => $sch) {
      if (woolman_aval($sch, 'country_id') != $sch_country) {
        continue;
      }
      $schools[$id] = $sch['organization_name'];
      if (woolman_aval($sch, 'city')) {
        $schools[$id] .= ', ' . $sch['city'];
      }
    }
    $schools['none'] = '- Other: Please Specify -';
  }
  else {
    $schools['none'] = '- None Found: Please Enter Your School -';
  }

  $form = array();
  $form['#prefix'] = '<p>' . _ws_apply_settings('ws_apply_school_instructions', TRUE) . '</p>';

  $form['school'] = array(
    '#type' => 'fieldset',
    '#title' => 'Educational Background',
  );
  $form['school']['school_type'] = array(
    '#type' => 'select',
    '#title' => 'I am',
    '#options' => array(
      'current' => 'Currently Enrolled in High School',
      'grad' => 'A recent High School Graduate',
      'other' => 'Homeschooled / Other'),
    '#description' => 'Choose the option that best describes your schooling',
    '#default_value' => ($saved ? $saved['school_type'] : $type_default),
  );
  $form['school']['graduation_date'] = array(
    '#type' => 'date',
    '#title' => 'Graduation Date',
    '#default_value' => ($saved ? $saved['graduation_date'] : $grad_default),
    '#description' => 'Date you will (or did) finish your High School studies'
  );
  $form['school']['school_options'] = array(
    '#prefix' => '<div id="school-options" class="subset">',
    '#suffix' => '</div>'
  );
  $form['school']['school_options']['school_location'] = array(
    '#title' => 'Your School\'s State/Country',
    '#type' => 'select',
    '#options' => $states_countries,
    '#default_value' => $location,
    '#ahah' => array(
      'path' => '~semester/admissions/apply/js',
      'event' => 'change',
      'wrapper' => 'school-choice-wrapper',
      'method' => 'replace',
      'effect' => 'fade',
    ),
  );
  $form['school']['school_options']['schools_button'] = array(
    '#type' => 'submit',
    '#value' => 'Change Location',
    '#attributes' => array('class' => 'next-button'),
    '#submit' => array('_ws_apply_form_ahah_submit'),
  );
  $form['school']['school_options']['school_choice_wrapper'] = array(
  '#prefix' => '<div id="school-choice-wrapper">',
  '#suffix' => '</div>'
  );
  $form['school']['school_options']['school_choice_wrapper']['school_choice'] = array(
    '#title' => 'Found '. (count($schools)-1) .' Schools in '.str_replace('USA: ', '', $states_countries[$location]),
    '#type' => 'select',
    '#options' => $schools,
    '#default_value' => ($saved ? $saved['school_choice'] : $school['contact_id']),
    '#description' => 'We may already have your school in our records. If not, choose "other" at the bottom of the list.'
  );
  if (count($schools) == 1) {
    $form['school']['school_options']['school_choice_wrapper']['school_choice']['#default_value'] = 'none';
    unset($form['school']['school_options']['school_choice_wrapper']['school_choice']['#description']);
  }
  $form['school']['details'] = array(
    '#prefix' => '<div id="school-details" style="clear:both;">',
    '#suffix' => '</div>'
  );
  $form['school']['details']['school_name'] = array(
    '#type' => 'textfield',
    '#title' => 'Your High School',
    '#size' => 45,
    '#description' => 'Please enter the full name of your school.',
    '#default_value' => woolman_aval($saved, 'school_name'),
  );
  $form['school']['details']['school_website'] = array(
    '#prefix' => '<div class="subset">',
    '#type' => 'textfield',
    '#title' => 'School Website',
    '#default_value' => woolman_aval($saved, 'school_website'),
    '#size' => 20,
  );
  $form['school']['details']['school_phone'] = array(
    '#type' => 'textfield',
    '#title' => 'Phone number',
    '#size' => 20,
    '#suffix' => '</div>',
    '#default_value' => woolman_aval($saved, 'school_phone'),
  );
  $form['school']['details']['school_address'] = array(
    '#type' => 'textfield',
    '#title' => 'Street Address',
    '#default_value' => woolman_aval($saved, 'school_address'),
  );
  $form['school']['details']['school_city'] = array(
    '#prefix' => '<div class="subset">',
    '#type' => 'textfield',
    '#title' => 'City',
    '#size' => 15,
    '#default_value' => ($saved ? $saved['school_city'] : $student['city']),
  );
  $form['school']['details']['school_zip'] = array(
    '#type' => 'textfield',
    '#title' => 'Zip/Post Code',
    '#size' => 10,
    '#suffix' => '</div>',
    '#default_value' => woolman_aval($saved, 'school_zip'),
  );
  $form['school']['education'] = array(
    '#prefix' => '<div id="school-other" style="clear:both;">',
    '#suffix' => '</div>',
    '#type' => 'textarea',
    '#title' => 'Describe your educational background',
    '#default_value' => woolman_aval($saved, 'education'),
    '#rows' => 10,
  );
  for ($i=1; $i<=2; $i++) {
    $form['teacher' . $i] = array(
      '#type' => 'fieldset',
      '#title' => 'Teacher ' . $i,
      '#attributes' => array('class' => 'two-up num-' . $i),
      '#description' => '<p>We will email this person a teacher recommendation form</p>',
    );
    if ($i == 1) {
      $form['teacher' . $i]['#prefix'] = '<p>' .  _ws_apply_settings('ws_apply_school2_instructions') . '</p><div style="clear:both">';
    }
    if ($i == 2) {
      $form['teacher' . $i]['#suffix'] = '</div>';
    }
    $form['teacher' . $i]['teacher' . $i . '_first'] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#title' => 'First name',
      '#default_value' => woolman_aval($saved, 'teacher' . $i . '_first'),
      '#size' => 15,
      '#req' => 1,
    );
    $form['teacher' . $i]['teacher' . $i . '_last'] = array(
      '#type' => 'textfield',
      '#title' => 'Last name',
      '#default_value' => woolman_aval($saved, 'teacher' . $i . '_last'),
      '#size' => 20,
      '#req' => 1,
      '#suffix' => '</div>',
    );
    $form['teacher' . $i]['teacher' . $i . '_email'] = array(
      '#type' => 'textfield',
      '#title' => 'Email Address',
      '#default_value' => woolman_aval($saved, 'teacher' . $i . '_email'),
      '#size' => 20,
      '#req' => 1,
      '#prefix' => '<div class="subset">',
    );
    $form['teacher' . $i]['teacher' . $i . '_email_test'] = array(
      '#type' => 'textfield',
      '#title' => 'Retype Email',
      '#default_value' => woolman_aval($saved, 'teacher' . $i . '_email'),
      '#size' => 20,
      '#req' => 1,
      '#suffix' => '</div>',
    );
    $form['teacher' . $i]['teacher' . $i . '_relationship'] = array(
      '#type' => 'radios',
      '#title' => 'Teaches at',
      '#default_value' => woolman_aval($saved, 'teacher' . $i . '_relationship'),
      '#options' => array('school' => 'Your School', 'other' => 'Other:'),
      '#prefix' => '<div class="subset teaches-at">',
      '#mark_req' => TRUE,
    );
    $form['teacher' . $i]['teacher' . $i . '_employer_other'] = array(
      '#type' => 'textfield',
      '#suffix' => '</div>',
      '#title' => 'Specify Other',
      '#size' => 22,
      '#default_value' => woolman_aval($saved, 'teacher' . $i . '_employer_other'),
    );
    $form['teacher' . $i]['teacher' . $i . '_gender'] = array(
      '#type' => 'radios',
      '#title' => 'Gender',
      '#default_value' => woolman_aval($saved, 'teacher' . $i . '_gender'),
      '#options' => array(1=> 'Female', 2=> 'Male'),
      '#req' => 1,
      '#prefix' => '<div class="subset">',
    );
    $form['teacher' . $i]['teacher' . $i . '_phone'] = array(
      '#type' => 'textfield',
      '#title' => 'Phone',
      '#req' => 1,
      '#size' => 13,
      '#default_value' => woolman_aval($saved, 'teacher' . $i . '_phone'),
    );
    $form['teacher' . $i]['teacher' . $i . '_phone_location'] = array(
      '#type' => 'select',
      '#title' => 'At',
      '#options' => array(2 => 'Work', 1 => 'Home', 4 => 'Cell'),
      '#default_value' => woolman_aval($saved, 'teacher' . $i . '_phone_location'),
    );
  }
  _ws_apply_add_buttons($form, 'school');
  $form['buttons']['submit']['#validate'][] = '_ws_apply_form_school_validate';

  return $form;
}

/**
 * Short-answer Questions Form
 */
function _ws_apply_form_type_questions($form_state) {
  drupal_add_css('sites/all/libraries/jquery.ui/themes/default/ui.all.css');
  drupal_add_css(drupal_get_path('module', 'woolman_semester') . '/css/ws_apply_online_forms.css');
  jquery_ui_add(array('ui.dialog'));
  drupal_add_js(drupal_get_path('module', 'woolman_semester') . '/js/ws_apply_forms.js');
  $form = array();
  $form['#prefix'] = '<p>' . _ws_apply_settings('ws_apply_questions_instructions', TRUE) . '</p>';

  for ($i=1; $i<=4; $i++) {
    $question = _ws_apply_settings('ws_apply_question' . $i);
    if ($question)
      $form['ws_apply_question' . $i] = array(
      '#type' => 'textarea',
      '#title' => $question,
      '#rows' => 11,
      '#req' => TRUE,
   );
  }
  _ws_apply_add_buttons($form, 'questions', 'Submit Answers');
  return _ws_apply_read_draft($form, 40);
}

/**
 * Essay Form
 */
function _ws_apply_form_type_essay($form_state) {
  drupal_add_css('sites/all/libraries/jquery.ui/themes/default/ui.all.css');
  drupal_add_css(drupal_get_path('module', 'woolman_semester') . '/css/ws_apply_online_forms.css');
  jquery_ui_add(array('ui.dialog'));
  drupal_add_js(drupal_get_path('module', 'woolman_semester') . '/js/ws_apply_forms.js');

  //Load essay choices
  $form = $choices = array();
  for ($i=1; $i<=4; $i++) {
    if ($c = _ws_apply_settings('ws_apply_essay' . $i)) {
      $choices[$i] = $c;
    }
  }
  $form['#prefix'] = '<p>' . _ws_apply_settings('ws_apply_essay_instructions', TRUE) . '</p>';

  if (count($choices) > 1)
    $form['choice'] = array(
      '#type' => 'radios',
      '#title' => 'Choose an Essay Question',
      '#options' => $choices,
      '#req' => 1,
      '#attributes' => array('class' => 'essay-choices')
   );
  else {
    $form['#prefix'] .= '<h5>' . $choices[1].'</h5>';
  }

  $form['essay'] = array(
    '#type' => 'textarea',
    '#title' => 'Essay',
    '#description' => 'Type or paste your essay answer here.',
    '#req' => TRUE,
    '#rows' => 25,
  );
  $form['format'] = filter_form();

  _ws_apply_add_buttons($form, 'essay', 'Submit Essay');
  return _ws_apply_read_draft($form, 36);
}

/**
 * Parent/Teacher Reference Form
 */
function _ws_apply_form_type_reference(&$form_state) {
  $arr = explode('-', $_GET['id']);
  $form_state['storage']['student'] = $student = woolman_contact_get($arr[0]);
  $form_state['storage']['self'] = $self = woolman_contact_get($arr[1]);
  $form_state['storage']['id'] = $_GET['id'];

  // Get most recent version of the activity
  $act = $form_state['storage']['act'] = ws_admissions_fetch_current_act($arr[2]);

  //check if they have already completed the form
  if ($act['status_id'] == 2) {
    drupal_goto('~semester/admissions/reference/thank-you', 'id=' . $_GET['id']);
  }

  switch($act['activity_type_id']) {
    case 38:
      $rec = 'parent';
      $qs = 12;
      drupal_set_title('Parent/Guardian Questionnaire for ' . $student['display_name']);
      break;
    case 39:
      $rec = 'teacher';
      $qs = 6;
      drupal_set_title('Teacher Recommendation for ' . $student['display_name']);
      break;
    default:
      return NULL;
  }

  $instructions = _ws_apply_settings('ws_apply_' . $rec . '_instructions', TRUE);
  $form = array('#prefix' => '<p>'._ws_apply_replace_tokens($instructions, $student, $self).'</p>');

  for ($i=1; $i<=$qs; $i++) {
    if ($question = _ws_apply_settings('ws_apply_' . $rec . $i)) {
      $form['ws_apply_' . $rec . $i]  = array(
        '#type' => 'textarea',
        '#title' => _ws_apply_replace_tokens($question, $student, $self),
        '#rows' => 11,
        '#req' => TRUE,
      );
    }
  }
  if ($rec == 'teacher') {
    $form['rubric'] = array();
    for ($i=1; $i<=6; $i++) {
      if ($question = _ws_apply_settings('ws_apply_rubric' . $i)) {
        $form['rubric']['#type'] = 'fieldset';
        $form['rubric']['#title'] = 'Evaluation Rubric';
        $form['rubric']['#description'] = '<p><em>Please rate the following questions on a scale of 1 (least) to 5 (most):</em></p>';
        $s = explode(',', _ws_apply_settings('ws_apply_rubric_scale' . $i));
        $form['rubric']['ws_apply_rubric' . $i]  = array(
          '#type' => 'radios',
          '#prefix' => '<div class="subset">',
          '#suffix' => '</div>',
          '#title' => _ws_apply_replace_tokens($question, $student, $self),
          '#options' => array(1=> '1 ' . $s[0], 2=> '2' ,3=> '3 ' . $s[1], 4=> '4', 5=> '5 ' . $s[2]),
          '#req' => TRUE,
        );
      }
    }
  }
  $mailing_list = TRUE;
  $db = db_query("SELECT status FROM {civicrm_group_contact} WHERE group_id = 2 AND contact_id = %d", $self['contact_id']);
  if ($existing = db_fetch_object($db)) {
    $mailing_list = $existing->status != 'Removed';
  }
  $happy = 'We\'d be happy to email you our quarterly newsletter, with stories, ideas, and news from Woolman.';
  $unhappy = 'OK, we won\'t email you our newsletter.';
  $form['mailing_list'] = array(
    '#type' => 'checkbox',
    '#title' => 'Receive the Woolman newsletter',
    '#default_value' => $mailing_list,
    '#description' => $mailing_list ? $happy : $unhappy,
    '#attributes' => array('onchange' => 'if ($(this).is(":checked")) { $("#edit-mailing-list-wrapper .description").html("' . $happy . '"); }else { $("#edit-mailing-list-wrapper .description").html("' . $unhappy . '"); }'),
  );
  _ws_apply_add_buttons($form, 'reference');
  return _ws_apply_read_draft($form, 0, $arr[2]);
}

/**
 * Financial Aid Form
 */
function _ws_apply_form_type_financial_aid(&$form_state) {
  drupal_set_title('Financial Aid');
  $case = _ws_apply_get_case();
  $form = array();
  $form['financial'] = array(
    '#type' => 'radios',
    '#options' => array(1 => 'Yes', 0 => 'No'),
    '#title' => 'Will you be applying for financial aid?',
    '#required' => TRUE,
  );
  if ($aid = woolman_aval($case['acts'], "51:0:id")) {
    $act = woolman_civicrm_api('activity', 'get', array('return.custom_61' => 1, 'id' => $aid), 'single');
    $form['financial']['#default_value'] = woolman_aval($act, 'custom_61', NULL, TRUE);
    $form_state['storage']['act'] = $act;
  }
  else {
    $result = woolman_civicrm_api('activity', 'create', array(
      'source_contact_id' => woolman_user_cid(),
      'status_id' => 6,
      'medium_id' => 7,
      'case_id' => $case['id'],
      'activity_type_id' => 51,
    ));
    $form_state['storage']['act'] = array('id' => $result['id'], 'status_id' => 6);
  }
  $form['instructions'] = array(
    '#value' => _ws_apply_settings('ws_apply_financial_aid_instructions'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Submit',
  );
  return $form;
}

/**
 * Semester preference Form
 */
function _ws_apply_form_semester_pref(&$form_state) {
  $s = $form_state['storage'] = woolman_semester_pref(woolman_user_cid());

  $form = array(
    'pref' => array(
      '#prefix' => '<div id="semester-pref-form">',
      '#suffix' => '</div>',
      '#value' => ' ',
    ),
  );
  $withdrawn = (!$s['status_id'] || !$s['semester'] || $s['status_id'] == 4);
  // Show prefs
  if (!($vals = woolman_aval($form_state, 'values'))) {
    $form['pref']['#value'] = 'You are applying';
    if ($withdrawn) {
      $form['pref']['#value'] = 'You have withdrawn your application';
    }
    elseif ($s['status_id'] == 2) {
      $form['pref']['#value'] = count($s['semester']) < 2 ? 'You are enrolled' : 'Congratulations, you have been accepted';
    }
    if ($s['semester']) {
      $form['pref']['#value'] .= ' for <strong>' . implode('</strong> or <strong>', $s['semester']) . '</strong>';
    }
    if (!($s['status_id'] == 2 && count($s['semester']) < 2)) {
      $form['pref']['edit'] = array(
        '#type' => 'submit',
        '#value' => 'Edit',
        '#ahah' => array(
          'path' => '~semester/admissions/apply/js',
          'wrapper' => 'semester-pref-form',
          'method' => 'replace',
          'effect' => 'fade',
        ),
      );
    }
  }
  // Show form
  else {
    $form['pref']['semester_choices'] = array(
      '#type' => 'checkboxes',
      '#description' => $withdrawn ? 'Pick a semester and click save to reinstate your application.' : 'Which semester(s) are you applying for? (choose none to withdraw your application)',
      '#prefix' => '<div class="subset">',
      '#suffix' => '</div>',
      '#options' => woolman_upcoming_semesters(4),
      '#default_value' => array_keys($s['semester']),
    );
    $form['pref']['save'] = array(
      '#type' => 'submit',
      '#value' => 'Save',
    );
  }
  return $form;
}

/**
 * A custom "required field" validator to accomodate "back" and "save draft" buttons
 * TODO: Use #limit_validation_errors in D7
 */
function _ws_apply_submit_validate($form, &$form_state) {
  switch ($form['#id']) {
    case '-ws-apply-form-type-form':
      $js = 'ws_apply_form_basics';
      break;
    case '-ws-apply-form-type-school':
      $js = 'ws_apply_form_school';
      break;
    default:
      $js = 'ws_apply_forms';
  }
  drupal_add_js(drupal_get_path('module', 'woolman_semester') . '/js/' . $js . '.js', 'module', 'header', TRUE, FALSE);
  $required = array();

  foreach ($form as $field=> $value) {
    if (!$value['#type'] || $value['#type'] == 'hidden' || !is_array($value) || strpos($field, '#')!==FALSE)
      continue;
    if ($value['#type'] == 'fieldset') {
      foreach ($value as $f=> $v) {
        if (!$v['#type'] || $v['#type'] == 'hidden' || !is_array($v) || strpos($field, '#')!==FALSE)
          continue;
        if ($v['#req'] && is_array($value))
          $required[$f] = $value['#title'].': ' . $v['#title'];
      }
    }
    elseif ($value['#req'])
      $required[$field] = $value['#title'];
  }

  foreach ($required as $field=> $title) {
    if (!$form_state['values'][$field]) {
      form_set_error($field, 'Please fill in the ' . $title);
    }
  }
}

function _ws_apply_form_basic_validate($form, &$form_state) {
  $vals = $form_state['values'];

  //For each person, check that they entered at least one phone number and a real birth date
  foreach (array('self', 'parent1', 'parent2') as $name) {
    if ($name == 'parent2' && !$vals['parent2_first']) {
      break;
    }
    if (!$vals[$name.'_home_phone'] && !$vals[$name.'_cell_phone'] && empty($vals[$name.'_work_phone'])) {
      form_set_error($name.'_home_phone', 'Please provide at least one phone number.');
    }
    if ($vals[$name . '_dob']['year'] >= date('Y') || $vals[$name . '_dob']['year'] < 1911) {
      form_set_error($name . '_dob', 'Please enter a birth date');
    }
    if ($name != 'self' && $vals[$name.'_email'] != $vals[$name.'_email_test']) {
      form_set_error($name . '_email', 'Email mismatch');
    }
  }

  //Require address fields if not same as student
  $required = array();
  if (!$vals['parent1_same']) {
    $required = drupal_map_assoc(array('parent1_street', 'parent1_city', 'parent1_state', 'parent1_zip'));
  }
  if (!$vals['parent2_same'] && $vals['parent2_first']) {
    $required += drupal_map_assoc(array('parent2_street', 'parent2_city', 'parent2_state', 'parent2_zip'));
  }
  if ($vals['parent2_first']) {
    $required += drupal_map_assoc(array('parent2_last'));
  }

  foreach ($required as $req) {
    if (!$vals[$req]) {
      form_set_error($req, 'Please complete all required fields');
    }
  }
}

function _ws_apply_form_school_validate($form, &$form_state) {
  $vals = $form_state['values'];

  for ($i=1; $i<=2; $i++) {
    if (!$vals['teacher' . $i . '_employer_other'] && ($vals['teacher' . $i . '_relationship'] == 'other' || $vals['school_type'] == 'other')) {
      form_set_error('teacher' . $i . '_employer_other', 'Please fill in the Teacher ' . $i . ': Employer');
    }
    if (!$vals['teacher' . $i . '_relationship'] && $vals['school_type'] != 'other') {
      form_set_error('teacher' . $i . '_relationship', 'Please fill in the Teacher ' . $i . ': Teaches At');
    }
    if ($vals['teacher' . $i . '_email'] != $vals['teacher' . $i . '_email_test']) {
      form_set_error('teacher' . $i . '_email', 'Email mismatch');
    }
    elseif (!strpos($vals['teacher' . $i . '_email'], '@')) {
      form_set_error('teacher' . $i . '_email', 'Enter a valid email');
    }
  }
  if ($vals['graduation_date']['year'] < date('Y') - 5) {
    form_set_error('graduation_date', 'Please enter your graduation date.');
  }
  if (!$vals['education'] && $vals['school_type'] == 'other') {
    form_set_error('education', 'Please describe your educational background.');
  }
}

function _ws_apply_add_buttons(&$form, $type, $submit_text = 'Submit Form') {
  $form['buttons'] = array(
    '#prefix' => '<div style="clear:both;">',
    '#suffix' => '</div>',
    'submit' => array(
      '#type' => 'submit',
      '#value' => $submit_text,
      '#submit' => array('_ws_apply_form_' . $type . '_submit'),
      '#validate' => array('_ws_apply_submit_validate'),
    ),
    'save' => array(
      '#type' => 'submit',
      '#value' => 'Save as Draft',
      '#submit' => array('_ws_apply_save_draft'),
    ),
  );
  $form['#suffix'] = '<div id="view-confirmation" style="display:none">
                          <div id="confirmation-inner"></div>
                      </div>';
}

/**
 * Check for a saved draft of this form, and set default values if one exists
 */
function _ws_apply_read_draft($form, $type, $id = NULL) {
  if (!$id) {
    $case = _ws_apply_get_case();
    //use original_id if it exists for compatability with civi activity revisioning system
    if (!($id = woolman_aval($case, "acts:$type:0:original_id", woolman_aval($case, "acts:$type:0:id")))) {
      return $form;
    }
  }

  $query = db_query('SELECT content FROM {ws_apply_online_drafts} WHERE id = %d', $id);
  if ($serial = db_fetch_object($query)) {
    $draft = unserialize($serial->content);
    woolman_fill_form_values($form, $draft);
  }
  return $form;
}
