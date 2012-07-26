<?php
require_once DRUPAL_ROOT . '/sites/all/libraries/jpgraph/src/jpgraph.php';

function ws_prospectives_dashboard($reps) {
  //call functions to update woolman_graphs if needed
  $total = ws_admissions_db_by_rep($reps);
  ws_admissions_db_followup_now($total);
  ws_admissions_db_facebook($total);
  ws_admissions_db_new_cases();
  ws_admissions_db_followups($reps);
  ws_admissions_db_student_acts();

  return
    '<div id="admissions-dashboard">
      <h2>Current Admissions Statistics</h2>
      <div class="db-stack">
          <img src="/sites/default/files/woolman_graphs/by_rep.png" alt="Prospectives by Admissions Rep" />
          <img src="/sites/default/files/woolman_graphs/facebook.png" alt="Facebook Friends" />
          <img src="/sites/default/files/woolman_graphs/followup_now.png" alt="Followups In Past Month" />
      </div>
      <div class="db-stack">
        <img src="/sites/default/files/woolman_graphs/new_cases.png" alt="New Prospective Students" />
        <img src="/sites/default/files/woolman_graphs/student_acts.png" alt="Communication From Students" />
      </div>
      <div class="db-graph">
        <img src="/sites/default/files/woolman_graphs/followups.png" alt="Follow-ups by Staff" />
      </div>
    </div>';
}

/**
 * Pie chart of admissions reps
 */
function ws_admissions_db_by_rep($reps) {
  $db = db_query("
    SELECT contact_id_b AS id, COUNT(contact_id_a) AS students
    FROM {civicrm_relationship}
    WHERE relationship_type_id = 13 AND is_active = 1 AND case_id IN (SELECT id FROM {civicrm_case} WHERE case_type_id = '1' AND is_deleted = 0 AND status_id IN (1,3))
    GROUP BY contact_id_b"
  );
  $total = 0;
  while($row = db_fetch_array($db)) {
    $by_rep[] = (int)$row['students'];
    $by_rep_label[] = $reps[$row['id']]."\n" . $row['students'];
    $total += $row['students'];
  }

  if($by_rep != cache_get('ws_prospectives_dashboard_by_rep')->data) {
    require_once DRUPAL_ROOT . '/sites/all/libraries/jpgraph/src/jpgraph_pie.php';
    require_once DRUPAL_ROOT . '/sites/all/libraries/jpgraph/src/jpgraph_pie3d.php';

    $graph = new PieGraph(175,156);

    $theme_class = new SoftyTheme;
    $graph->SetTheme($theme_class);


    $graph->title->Set("$total Total Prospectives");
    $graph->title->SetFont(FF_DV_SANSSERIF,FS_BOLD,7);

    $graph->SetFrame(true,'#D0E7A8',1);
    $graph->SetMarginColor('#FAFFDC');

    $p1 = new PiePlot3D($by_rep);

    $p1->SetAngle(70);
    $p1->ExplodeAll(3);

    $p1->SetLabels($by_rep_label);
    $p1->SetLabelPos(0.3);

    $graph->Add($p1);
    $graph->Stroke('sites/default/files/woolman_graphs/by_rep.png');

    cache_set('ws_prospectives_dashboard_by_rep', $by_rep);
  }
  return $total;
}

/**
 * Pie Chart of current followups
 */
function ws_admissions_db_followup_now($total) {
  $db = db_query("
    SELECT COUNT(id) AS acts
    FROM {civicrm_case} cas
    WHERE cas.case_type_id = '1' AND cas.is_deleted = 0 AND cas.status_id IN (1,3) AND cas.id IN (
      SELECT ca.case_id FROM {civicrm_case_activity} ca, {civicrm_activity} act
      WHERE act.is_current_revision = 1  AND act.is_deleted = 0 AND act.activity_type_id IN (14,29) AND act.status_id IN (2,3) AND act.activity_date_time >= (date_sub(curdate(),interval 1 month)) AND act.id = ca.activity_id)"
  );
  $row = db_fetch_array($db);
  $followup_now = array($row['acts'], $total-$row['acts']);

  if($followup_now != cache_get('ws_prospectives_dashboard_followup_now')->data) {
    require_once DRUPAL_ROOT . '/sites/all/libraries/jpgraph/src/jpgraph_pie.php';
    require_once DRUPAL_ROOT . '/sites/all/libraries/jpgraph/src/jpgraph_pie3d.php';

    $graph = new PieGraph(175,156);

    $theme_class = new SoftyTheme;
    $graph->SetTheme($theme_class);

    $graph->title->Set($followup_now[0].' of ' . $total." were followed-\nup in the past 30 days");
    $graph->title->SetFont(FF_DV_SANSSERIF,FS_BOLD,7);

    $graph->SetFrame(true,'#D0E7A8',1);
    $graph->SetMarginColor('#FAFFDC');

    $p1 = new PiePlot3D($followup_now);

    $p1->SetAngle(70);
    $p1->ExplodeAll(3);

    $p1->SetLabels(array("Recently\nContacted", ""));
    $p1->SetLabelPos(0.3);

    $graph->Add($p1);
    $graph->Stroke('sites/default/files/woolman_graphs/followup_now.png');

    cache_set('ws_prospectives_dashboard_followup_now', $followup_now);
  }
}

/**
 *Pie Chart of facebook fans
 */
function ws_admissions_db_facebook($total) {
  $db = db_query("
    SELECT COUNT(id) AS acts
    FROM {civicrm_case} cas
    WHERE cas.case_type_id = '1' AND cas.is_deleted = 0 AND cas.status_id IN (1,3) AND cas.id IN (
      SELECT ca.case_id FROM {civicrm_case_activity} ca, {civicrm_activity} act
      WHERE act.is_current_revision = 1  AND act.is_deleted = 0 AND act.activity_type_id = 31 AND act.status_id = 2 AND act.id = ca.activity_id)");
  $row = db_fetch_array($db);
  $facebook = array($row['acts'], $total-$row['acts']);

  if($facebook != cache_get('ws_prospectives_dashboard_facebook')->data) {
    require_once DRUPAL_ROOT . '/sites/all/libraries/jpgraph/src/jpgraph_pie.php';
    require_once DRUPAL_ROOT . '/sites/all/libraries/jpgraph/src/jpgraph_pie3d.php';

    $graph = new PieGraph(175,156);

    $theme_class = new SoftyTheme;
    $graph->SetTheme($theme_class);

    $graph->title->Set($facebook[0].' of ' . $total." are FB Friends");
    $graph->title->SetFont(FF_DV_SANSSERIF,FS_BOLD,7);

    $graph->SetFrame(true,'#D0E7A8',1);
    $graph->SetMarginColor('#FAFFDC');

    $p1 = new PiePlot3D($facebook);

    $p1->SetAngle(70);
    $p1->ExplodeAll(3);

    $p1->SetLabels(array("Friendship\nConfirmed", ''));
    $p1->SetLabelPos(0.3);

    $graph->Add($p1);
    $graph->Stroke('sites/default/files/woolman_graphs/facebook.png');

    cache_set('ws_prospectives_dashboard_facebook', $facebook);
  }
}

/**
 * Bar Graph of new prospective students by month
 */
function ws_admissions_db_new_cases() {
  $db = db_query("
    SELECT start_date, COUNT(id) AS students
    FROM {civicrm_case}
    WHERE case_type_id = '1' AND start_date >= (date_sub(curdate(),interval 1 year)) AND is_deleted <> 1
    GROUP BY YEAR(start_date), MONTH(start_date)");
  while($row = db_fetch_array($db)) {
    $new_cases_data[substr($row['start_date'], 0, 7)] = $row['students'];
  }

  if($new_cases_data != cache_get('ws_prospectives_dashboard_new_cases')->data) {
    for ($i=11; $i>=0; --$i) {
      $m = date_create("now - $i month");
      $date = date_format($m, 'Y-m');
      $new_cases_label[] = ($i==0?'  ':'').date_format($m, 'M').($i==11||$i==0 ? "\n".date_format($m, 'Y') : '');
      if($new_cases_data[$date])
        $new_cases[] = (int)$new_cases_data[$date];
      else
        $new_cases[] = 0;
    }
    require_once DRUPAL_ROOT . '/sites/all/libraries/jpgraph/src/jpgraph_bar.php';

    $graph = new Graph(340,240);
    $graph->SetScale('textlin');
    $graph->ygrid->SetWeight(0);


    $graph->SetMargin(37, 17, 25, 48);
    $graph->SetMarginColor('#FAFFDC');

    $graph->title->Set("New Prospective Students");
    $graph->title->SetFont(FF_DV_SANSSERIF,FS_BOLD);
    $graph->xaxis->SetTickLabels($new_cases_label);
    $graph->SetFrame(true,'#D0E7A8',1);

    $bplot = new BarPlot($new_cases);
    $bplot->SetFillGradient('#90C268', '#D9CD2F', GRAD_HOR);

    $graph->Add($bplot);
    $graph->Stroke('sites/default/files/woolman_graphs/new_cases.png');

    cache_set('ws_prospectives_dashboard_new_cases', $new_cases_data);
  }
}

/**
 * Graph of monthly followups by rep
 */
function ws_admissions_db_followups($reps) {
  unset($reps[0]);
  $rep_query = 'COUNT(IF(act.source_contact_id NOT IN ('.implode(',', array_keys($reps)).'),1,NULL)) AS acts_0';
  foreach($reps as $rep_id => $rep_name) {
    $rep_query .= ', COUNT(IF(act.source_contact_id=' . $rep_id.',1,NULL)) AS acts_' . $rep_id;
  }
  $db = db_query("
    SELECT act.activity_date_time AS date, $rep_query
    FROM {civicrm_activity} act
    WHERE activity_date_time >= (date_sub(curdate(),interval 1 year)) AND is_current_revision = 1 AND status_id IN (2,3) AND is_deleted = 0 AND activity_type_id IN (14,29) AND id IN (SELECT activity_id FROM {civicrm_case_activity} ca, {civicrm_case} cas WHERE cas.id = ca.case_id AND cas.case_type_id = '1' AND cas.is_deleted = 0)
    GROUP BY YEAR(act.activity_date_time), MONTH(act.activity_date_time)");
  $reps[0] = 'Others';
  while($row = db_fetch_array($db)) {
    foreach($reps as $rep_id => $rep_name) {
      $followups_data[substr($row['date'], 0, 7)][$rep_id] = $row['acts_' . $rep_id];
      $totals[$rep_id] += $row['acts_' . $rep_id];
    }
  }
  //if data has changed, rebuild graph
  if($totals != cache_get('ws_prospectives_dashboard_followups')->data) {

    //put insignificant data into "others" category
    foreach($totals as $rep_id => $total) {
      if(!$rep_id) continue;
      if($total<20) {
        foreach($followups_data as $rep) {
          $rep[0] += $rep[$rep_id];
          $totals[0] += $rep[$rep_id];
        }
        unset($reps[$rep_id]);
      }
    }
    if(!$totals[0])
      unset($reps[0]);
    for ($i=11; $i>=0; --$i) {
      $m = date_create("now - $i month");
      $date = date_format($m, 'Y-m');
      $followups_label[] = ($i==0?'  ':'').date_format($m, 'M').($i==11||$i==0 ? "\n".date_format($m, 'Y') : '');
      foreach($reps as $rep_id => $rep_name) {
        if($followups_data[$date][$rep_id])
          $followups[$rep_id][] = (int)$followups_data[$date][$rep_id];
        else
          $followups[$rep_id][] = 0;
      }
    }
    require_once DRUPAL_ROOT . '/sites/all/libraries/jpgraph/src/jpgraph_line.php';

    $graph = new Graph(340,490);
    $graph->SetScale('textlin');
    $theme_class = new SoftyTheme;
    $graph->SetTheme($theme_class);

    $graph->SetMargin(37, 20, 25, 48);
    $graph->ygrid->SetWeight(0);
    $graph->xgrid->SetLineStyle('solid');
    $graph->xgrid->SetColor('#EFEFEF');

    $graph->title->Set("Follow-ups by Staff");
    $graph->title->SetFont(FF_DV_SANSSERIF,FS_BOLD);
    $graph->subtitle->Set('(Follow-ups, Info Sent & Messages Left)');
    $graph->subtitle->SetFont(FF_DV_SANSSERIF,FS_ITALIC,7);
    $graph->xaxis->SetTickLabels($followups_label);
    $graph->SetFrame(true,'#D0E7A8',1);
    $graph->SetMarginColor('#FAFFDC');



    foreach($reps as $rep_id => $rep_name) {
      $lplot = new LinePlot($followups[$rep_id]);
      $lplot->SetLegend($rep_name);
      $lplot->SetWeight(4);
      $graph->Add($lplot);
      unset($lplot);
    }

    $graph->Stroke('sites/default/files/woolman_graphs/followups.png');
    cache_set('ws_prospectives_dashboard_followups', $totals);
  }
}

/**
 * Graph of incoming student communication
 */
function ws_admissions_db_student_acts() {
  $db = db_query("
    SELECT activity_date_time AS date, COUNT(id) AS acts
    FROM {civicrm_activity} act
    WHERE activity_date_time >= (date_sub(curdate(),interval 1 year)) AND is_current_revision = 1 AND status_id = 2 AND is_deleted = 0 AND activity_type_id IN (28,31) AND id IN (SELECT activity_id FROM {civicrm_case_activity} ca, {civicrm_case} cas WHERE cas.id = ca.case_id AND cas.case_type_id = '1' AND cas.is_deleted = 0 AND cas.status_id IN (1,3))
    GROUP BY YEAR(activity_date_time), MONTH(activity_date_time)");
  while($row = db_fetch_array($db)) {
    $student_acts_data[substr($row['date'], 0, 7)] = $row['acts'];
  }

  if($student_acts_data != cache_get('ws_prospectives_dashboard_student_acts')->data) {
    for ($i=11; $i>=0; --$i) {
      $m = date_create("now - $i month");
      $date = date_format($m, 'Y-m');
      $student_acts_label[] = ($i==0?'  ':'').date_format($m, 'M').($i==11||$i==0 ? "\n".date_format($m, 'Y') : '');
      if($student_acts_data[$date])
        $student_acts[] = (int)$student_acts_data[$date];
      else
        $student_acts[] = 0;
    }
    require_once DRUPAL_ROOT . '/sites/all/libraries/jpgraph/src/jpgraph_line.php';

    $graph = new Graph(340,240);
    $graph->SetScale('textlin');
    $theme_class = new SoftyTheme;
    $graph->SetTheme($theme_class);

    $graph->SetMargin(37, 20, 25, 48);

    $lplot = new LinePlot($student_acts);
    $lplot->SetFillColor('#D4E7FC');
    $graph->Add($lplot);

    $graph->ygrid->SetWeight(0);
    $graph->xgrid->SetLineStyle('solid');
    $graph->xgrid->SetWeight(1);
    $graph->xgrid->SetColor('#EFEFEF');
    $graph->title->Set("Communication From Students");
    $graph->title->SetFont(FF_DV_SANSSERIF,FS_BOLD);
    $graph->subtitle->Set('(Incoming Communication & Facebook Friendships)');
    $graph->subtitle->SetFont(FF_DV_SANSSERIF,FS_ITALIC,7);
    $graph->xaxis->SetTickLabels($student_acts_label);
    $graph->SetFrame(true,'#D0E7A8',1);
    $graph->SetMarginColor('#FAFFDC');
    $graph->img->SetAntiAliasing();

    $graph->Stroke('sites/default/files/woolman_graphs/student_acts.png');

    cache_set('ws_prospectives_dashboard_student_acts', $student_acts_data);
  }
}
