Set-StrictMode -Version Latest

function Assert-NinhoProperty {
    param([object]$Object, [string]$Name, [string]$Context)
    if ($null -eq $Object -or $Object.PSObject.Properties.Name -notcontains $Name) {
        throw "Spike evidence missing $Name at $Context"
    }
}

function Assert-NinhoAllocator {
    param([object]$Allocator, [string]$Context)
    foreach ($name in 'baseline_bytes', 'final_bytes', 'max_abs_delta', 'exact_return') {
        Assert-NinhoProperty $Allocator $name $Context
    }
    if ($Allocator.baseline_bytes -ne 0 -or $Allocator.final_bytes -ne 0 -or
            $Allocator.max_abs_delta -ne 0 -or $Allocator.exact_return -ne $true) {
        throw "Spike evidence allocator is not an exact zero return at $Context"
    }
}

function Assert-NinhoAllocatorSamples {
    param([object]$Allocator, [bool]$StressProtocol, [string]$Context)
    foreach ($name in 'warmup_post_teardown', 'measured_post_teardown') {
        Assert-NinhoProperty $Allocator $name $Context
    }
    $warmup = @($Allocator.warmup_post_teardown)
    $measured = @($Allocator.measured_post_teardown)
    if ($StressProtocol) {
        if ($warmup.Count -ne 10 -or $measured.Count -ne 10 -or
                @($warmup | Where-Object { $_ -ne 0 }).Count -ne 0 -or
                @($measured | Where-Object { $_ -ne 0 }).Count -ne 0) {
            throw 'Spike evidence Stress allocator samples must be ten exact zeros'
        }
    } elseif ($warmup.Count -ne 0 -or $measured.Count -ne 0) {
        throw "Spike evidence unexpected allocator samples for $Context"
    }
}

function Get-NinhoCapabilityValue {
    param([object]$Row, [string]$Name)
    $matches = @($Row.values | Where-Object name -CEQ $Name)
    if ($matches.Count -ne 1) {
        throw "Spike evidence missing unique capability value $($Row.capability)/$Name"
    }
    return [double]$matches[0].value
}

function Get-NinhoScenarioMetric {
    param([object]$Scenario, [string]$Name)
    $matches = @($Scenario.metrics | Where-Object name -CEQ $Name)
    if ($matches.Count -ne 1) {
        throw "Spike evidence missing unique scenario metric $($Scenario.name)/$Name"
    }
    return [double]$matches[0].value
}

function Get-NinhoTailSummary {
    param([object[]]$Samples, [string]$Context)
    $values = @($Samples)
    if ($values.Count -ne 10 -or @($values | Where-Object { [double]$_ -le 0 }).Count -ne 0) {
        throw "Spike evidence $Context requires exactly ten warmup and measured samples with positive values"
    }
    $tail = @($values[5..9] | ForEach-Object { [double]$_ } | Sort-Object)
    return [pscustomobject]@{
        full_min = $tail[0]
        central_min = $tail[1]
        median = $tail[2]
        central_max = $tail[3]
        full_max = $tail[4]
    }
}

function Assert-NinhoNear {
    param([double]$Actual, [double]$Expected, [string]$Context)
    if ([double]::IsNaN($Actual) -or [double]::IsInfinity($Actual) -or
            [math]::Abs($Actual - $Expected) -gt 1e-12) {
        throw "Spike evidence derived summary mismatch at $Context"
    }
}

function Assert-NinhoMemoryCounter {
    param(
        [object]$Counter,
        [string]$Context,
        [switch]$IncludeInstant,
        [switch]$RequireExactPeak
    )
    foreach ($name in @(
            'available','stable','terminal_growth','baseline_last_bytes',
            'baseline_full_min_bytes','baseline_central_min_bytes',
            'baseline_median_bytes','baseline_central_max_bytes',
            'baseline_full_max_bytes','final_last_bytes','final_full_min_bytes',
            'final_central_min_bytes','final_median_bytes','final_central_max_bytes',
            'final_full_max_bytes','peak_bytes','growth_ratio',
            'warmup_trimmed_span_ratio','warmup_full_span_ratio',
            'measured_trimmed_span_ratio','measured_full_span_ratio',
            'warmup_samples','measured_samples')) {
        Assert-NinhoProperty $Counter $name $Context
    }
    if ($IncludeInstant) {
        Assert-NinhoProperty $Counter 'instant_growth_ratio' $Context
    }
    $warmup = @($Counter.warmup_samples)
    $measured = @($Counter.measured_samples)
    if (-not $Counter.available) {
        if ($warmup.Count -ne 0 -or $measured.Count -ne 0 -or
                $Counter.stable -or $Counter.terminal_growth) {
            throw "Spike evidence unavailable memory counter mismatch at $Context"
        }
        foreach ($property in $Counter.PSObject.Properties) {
            if (($property.Name.EndsWith('_bytes') -or
                    $property.Name.EndsWith('_ratio')) -and
                    [double]$property.Value -ne 0) {
                throw "Spike evidence unavailable memory counter mismatch at $Context"
            }
        }
        return 'unavailable'
    }
    if ($warmup.Count -ne 10 -or $measured.Count -ne 10) {
        throw "Spike evidence $Context requires exactly ten warmup and measured samples"
    }
    $baseline = Get-NinhoTailSummary $warmup "$Context/warmup"
    $final = Get-NinhoTailSummary $measured "$Context/measured"
    $expectedIntegers = @{
        baseline_last_bytes = [double]$warmup[-1]
        baseline_full_min_bytes = $baseline.full_min
        baseline_central_min_bytes = $baseline.central_min
        baseline_median_bytes = $baseline.median
        baseline_central_max_bytes = $baseline.central_max
        baseline_full_max_bytes = $baseline.full_max
        final_last_bytes = [double]$measured[-1]
        final_full_min_bytes = $final.full_min
        final_central_min_bytes = $final.central_min
        final_median_bytes = $final.median
        final_central_max_bytes = $final.central_max
        final_full_max_bytes = $final.full_max
    }
    foreach ($name in $expectedIntegers.Keys) {
        if ([double]$Counter.$name -ne [double]$expectedIntegers[$name]) {
            throw "Spike evidence derived summary mismatch at $Context/$name"
        }
    }
    $growth = [math]::Max(0.0, ($final.median - $baseline.median) / $baseline.median)
    $warmupTrimmed = ($baseline.central_max - $baseline.central_min) / $baseline.median
    $warmupFull = ($baseline.full_max - $baseline.full_min) / $baseline.median
    $measuredTrimmed = ($final.central_max - $final.central_min) / $final.median
    $measuredFull = ($final.full_max - $final.full_min) / $final.median
    Assert-NinhoNear $Counter.growth_ratio $growth "$Context/growth_ratio"
    Assert-NinhoNear $Counter.warmup_trimmed_span_ratio $warmupTrimmed "$Context/warmup_trimmed_span_ratio"
    Assert-NinhoNear $Counter.warmup_full_span_ratio $warmupFull "$Context/warmup_full_span_ratio"
    Assert-NinhoNear $Counter.measured_trimmed_span_ratio $measuredTrimmed "$Context/measured_trimmed_span_ratio"
    Assert-NinhoNear $Counter.measured_full_span_ratio $measuredFull "$Context/measured_full_span_ratio"
    if ($IncludeInstant) {
        $instant = [math]::Max(0.0, ([double]$measured[-1] - [double]$warmup[-1]) / [double]$warmup[-1])
        Assert-NinhoNear $Counter.instant_growth_ratio $instant "$Context/instant_growth_ratio"
    }
    $terminalThreshold = 1.05 * $baseline.median
    $terminalGrowth = [double]$measured[8] -gt $terminalThreshold -and
        [double]$measured[9] -gt $terminalThreshold
    $stable = $warmupTrimmed -le 0.05 -and $measuredTrimmed -le 0.05
    $sampleMaximum = [double](($warmup + $measured | Measure-Object -Maximum).Maximum)
    $peakMismatch = if ($RequireExactPeak) {
        [double]$Counter.peak_bytes -ne $sampleMaximum
    } else {
        [double]$Counter.peak_bytes -lt $sampleMaximum
    }
    if ($Counter.terminal_growth -ne $terminalGrowth -or $Counter.stable -ne $stable -or
            $peakMismatch) {
        throw "Spike evidence derived summary mismatch at $Context/flags"
    }
    if ($warmupTrimmed -gt 0.05) { return 'unstable' }
    if ($growth -gt 0.05 -or $terminalGrowth) { return 'growth' }
    if ($measuredTrimmed -gt 0.05) { return 'unstable' }
    return 'pass'
}

function Assert-NinhoMemoryObservation {
    param([object]$Memory, [string]$Context)
    if ($null -eq $Memory) {
        throw "Spike evidence memory observation missing at $Context"
    }
    foreach ($name in @(
            'gate_scope','gate_status','assessment_status','gate_applied',
            'budget_qualified','budget_scope','private_commit','working_set')) {
        Assert-NinhoProperty $Memory $name $Context
    }
    if ($Memory.gate_scope -cne 'release_mt' -or
            $Memory.gate_status -cne 'diagnostic' -or $Memory.gate_applied -or
            $Memory.budget_qualified -or
            $Memory.budget_scope -cne 'future_packaged_reference_hardware') {
        throw "Spike evidence memory gate contract mismatch at $Context"
    }
    $privateAssessment = Assert-NinhoMemoryCounter `
        -Counter $Memory.private_commit -Context "$Context/private_commit" `
        -IncludeInstant -RequireExactPeak
    if ($Memory.assessment_status -cne $privateAssessment) {
        throw "Spike evidence derived summary mismatch at $Context/private assessment"
    }
    $working = $Memory.working_set
    foreach ($name in @(
            'assessment_status','gate_status','gate_applied','budget_qualified','budget_scope')) {
        Assert-NinhoProperty $working $name "$Context/working_set"
    }
    if ($working.gate_status -cne 'diagnostic' -or $working.gate_applied -or
            $working.budget_qualified -or
            $working.budget_scope -cne 'future_packaged_reference_hardware') {
        throw "Spike evidence memory gate contract mismatch at $Context/working_set"
    }
    $workingAssessment = Assert-NinhoMemoryCounter `
        -Counter $working -Context "$Context/working_set" -IncludeInstant
    if ($working.assessment_status -cne $workingAssessment) {
        throw "Spike evidence derived summary mismatch at $Context/working assessment"
    }
}

function Assert-NinhoCrt {
    param([object]$Crt, [bool]$ExpectedApplicable, [string]$Context)
    foreach ($name in @(
            'applicable','balanced','normal_block_count_delta','normal_block_bytes_delta',
            'client_block_count_delta','client_block_bytes_delta')) {
        Assert-NinhoProperty $Crt $name $Context
    }
    if ($Crt.applicable -ne $ExpectedApplicable) {
        throw "Spike evidence CRT applicability mismatch at $Context"
    }
    if (-not $Crt.balanced -or $Crt.normal_block_count_delta -ne 0 -or
            $Crt.normal_block_bytes_delta -ne 0 -or
            $Crt.client_block_count_delta -ne 0 -or
            $Crt.client_block_bytes_delta -ne 0) {
        throw "Spike evidence CRT balance mismatch at $Context"
    }
}

function Assert-NinhoSpikeEvidenceDocument {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)] [object]$Document,
        [Parameter(Mandatory)]
        [ValidateSet('Debug', 'Release')]
        [string]$ExpectedBuildType
    )

    $requiredTopLevel = @(
        'schema', 'tool', 'dependencies', 'build_type', 'configuration',
        'process_box3d_allocator', 'budget_qualification', 'warnings',
        'scenarios', 'matrix', 'violations', 'recommendation')
    foreach ($name in $requiredTopLevel) {
        Assert-NinhoProperty $Document $name 'document'
    }
    if ($Document.schema -cne 'ninho.physics.scenario.v1' -or
            $Document.tool.name -cne 'ninho_physics_spike' -or
            $Document.tool.version -cne '0.1.0') {
        throw 'Spike evidence schema or tool identity mismatch'
    }
    $pins = [ordered]@{
        box3d = @('0.1.0', '8441b4a06d6d09dcfb0b0f704df4d847d1437b92')
        godot = @('4.5.1-stable', 'f62fdbde15035c5576dad93e586201f4d41ef0cb')
        godot_cpp = @('godot-4.5-stable', 'e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77')
    }
    foreach ($name in $pins.Keys) {
        Assert-NinhoProperty $Document.dependencies $name 'dependencies'
        $dependency = $Document.dependencies.$name
        if ($dependency.version -cne $pins[$name][0] -or
                $dependency.commit -cne $pins[$name][1]) {
            throw "Spike evidence dependency pin mismatch for $name"
        }
    }
    if ($Document.build_type -cne $ExpectedBuildType) {
        throw "Spike evidence build_type mismatch for $ExpectedBuildType"
    }
    if ($Document.configuration.seed -ne 1 -or
            $Document.configuration.substeps -ne 4 -or
            $Document.configuration.repeat -ne 2 -or
            [math]::Abs([double]$Document.configuration.time_step - (1.0 / 60.0)) -gt 1e-15) {
        throw 'Spike evidence execution configuration mismatch'
    }
    if (@($Document.violations).Count -ne 0 -or
            $Document.recommendation -cne 'prosseguir_com_limites') {
        throw 'Spike evidence has a normative failure'
    }
    if ($Document.budget_qualification.status -cne 'deferred' -or
            $Document.budget_qualification.warning -cne 'private_commit_budget_unqualified' -or
            [double]$Document.budget_qualification.target_growth_ratio -ne 0.05) {
        throw 'Spike evidence private commit budget is not explicitly deferred'
    }
    $globalWarnings = @($Document.warnings)
    if ($globalWarnings.Count -ne 1 -or
            $globalWarnings[0].code -cne 'private_commit_budget_unqualified' -or
            $globalWarnings[0].message -cne 'PrivateUsage budget is deferred to a packaged Release build on reference hardware') {
        throw 'Spike evidence global warning contract mismatch'
    }
    $globalWarningDetails = @($globalWarnings[0].details)
    if ($globalWarningDetails.Count -ne 1 -or
            $globalWarningDetails[0].name -cne 'budget_scope' -or
            $globalWarningDetails[0].value -cne 'future_packaged_reference_hardware') {
        throw 'Spike evidence global warning contract mismatch'
    }
    Assert-NinhoAllocator $Document.process_box3d_allocator 'process'
    Assert-NinhoAllocatorSamples $Document.process_box3d_allocator $false 'process'

    $expectedTopologies = [ordered]@{
        radial_fall = @(2, 2, 0, 1, 1)
        projectile_pile = @(123, 123, 0, 121, 221)
        radial_pile = @(81, 81, 0, 80, 204)
        mass_ratio = @(81, 81, 0, 80, 227)
        stress = @(500, 800, 250, 500, 0)
        capability_matrix = @(123, 123, 1, 121, 221)
    }
    $expectedFixtures = [ordered]@{
        radial_fall = @(1, 2, 0)
        projectile_pile = @(121, 123, 0)
        radial_pile = @(80, 81, 0)
        mass_ratio = @(80, 81, 0)
        stress = @(500, 800, 250)
        capability_matrix = @(0, 0, 0)
    }
    $expectedTicks = [ordered]@{
        radial_fall = 600
        projectile_pile = 11
        radial_pile = 1800
        mass_ratio = 1800
        stress = 15000
        capability_matrix = 21800
    }
    $expectedScenarioHashes = [ordered]@{
        radial_fall = '12100112409900625846'
        projectile_pile = '1431096453509785832'
        radial_pile = '10363635776067367757'
        mass_ratio = '17464060736204574665'
        stress = '10292935394449293550'
        capability_matrix = '17104053157009575930'
    }
    $expectedLimits = @{
        radial_fall = @(
            @{name='surface_separation_min'; comparison='>='; value=-0.02; unit='m'},
            @{name='surface_separation_max'; comparison='<='; value=0.03; unit='m'},
            @{name='final_linear_speed'; comparison='<'; value=0.05; unit='m/s'},
            @{name='final_angular_speed'; comparison='<'; value=0.1; unit='rad/s'},
            @{name='max_energy_growth_ratio'; comparison='<='; value=0.02; unit='ratio'})
        projectile_pile = @(
            @{name='primary_seed_passes'; comparison='=='; value=20; unit='seeds'},
            @{name='fallback_seed_passes'; comparison='=='; value=0; unit='seeds'},
            @{name='invalid_states'; comparison='=='; value=0; unit='states'})
        radial_pile = @(
            @{name='sleep_ratio'; comparison='>='; value=0.9; unit='ratio'},
            @{name='p95_linear_speed'; comparison='<'; value=0.05; unit='m/s'},
            @{name='p95_angular_speed'; comparison='<'; value=0.1; unit='rad/s'},
            @{name='max_penetration'; comparison='<'; value=0.02; unit='m'},
            @{name='spontaneous_speed'; comparison='<='; value=100; unit='m/s'},
            @{name='world_radius'; comparison='<='; value=60; unit='m'},
            @{name='energy_growth_from_tick_600'; comparison='<='; value=0.02; unit='ratio'})
        mass_ratio = @(
            @{name='sleep_ratio'; comparison='>='; value=0.8; unit='ratio'},
            @{name='p95_linear_speed'; comparison='<'; value=0.1; unit='m/s'},
            @{name='p95_angular_speed'; comparison='<'; value=0.2; unit='rad/s'},
            @{name='max_penetration'; comparison='<'; value=0.025; unit='m'},
            @{name='spontaneous_speed'; comparison='<='; value=100; unit='m/s'},
            @{name='world_radius'; comparison='<='; value=60; unit='m'})
        stress = @(
            @{name='allocator_warmup_cycles'; comparison='=='; value=10; unit='cycles'},
            @{name='scenario_timeout'; comparison='<='; value=60; unit='s'},
            @{name='spontaneous_speed'; comparison='<='; value=100; unit='m/s'})
        capability_matrix = @()
    }
    $expectedMetricNames = @{
        radial_fall = @('surface_separation','final_linear_speed','final_angular_speed','max_energy_growth_ratio','energy_window','energy_epsilon')
        projectile_pile = @('primary_seed_passes','fallback_seed_passes','projectile_speed','primary_invalid_states','fallback_invalid_states','fallback_seed_evaluated')
        radial_pile = @('sleep_ratio','p95_linear_speed','p95_angular_speed','max_penetration','max_platform_penetration','max_pair_penetration','minimum_density','maximum_density','energy_at_tick_600','max_energy_growth_ratio')
        mass_ratio = @('sleep_ratio','p95_linear_speed','p95_angular_speed','max_penetration','max_platform_penetration','max_pair_penetration','minimum_density','maximum_density','energy_at_tick_600','max_energy_growth_ratio')
        stress = @('private_commit_diagnostic_growth_target','private_commit_diagnostic_trimmed_span_target','working_set_diagnostic_growth_target','working_set_diagnostic_trimmed_span_target','private_commit_baseline_median_bytes','private_commit_final_median_bytes','private_commit_growth_ratio','private_commit_instant_growth_ratio','private_commit_warmup_trimmed_span_ratio','private_commit_measured_trimmed_span_ratio','working_set_baseline_bytes','working_set_baseline_low_bytes','working_set_peak_bytes','working_set_final_bytes','working_set_final_low_bytes','working_set_growth_ratio','working_set_instant_growth_ratio','executed_substeps','allocator_warmup_cycles','completed_cycles')
        capability_matrix = @()
    }
    $expectedMetricUnits = @{
        radial_fall = @('m','m/s','rad/s','ratio','ticks','J')
        projectile_pile = @('seeds','seeds','m/s','count','count','bool')
        radial_pile = @('ratio','m/s','rad/s','m','m','m','kg/m3','kg/m3','J','ratio')
        mass_ratio = @('ratio','m/s','rad/s','m','m','m','kg/m3','kg/m3','J','ratio')
        stress = @('ratio','ratio','ratio','ratio','bytes','bytes','ratio','ratio','ratio','ratio','bytes','bytes','bytes','bytes','bytes','ratio','ratio','count','cycles','count')
        capability_matrix = @()
    }
    $scenarios = @($Document.scenarios)
    if ($scenarios.Count -ne $expectedTopologies.Count) {
        throw 'Spike evidence scenario count mismatch'
    }
    foreach ($name in $expectedTopologies.Keys) {
        $matches = @($scenarios | Where-Object name -CEQ $name)
        if ($matches.Count -ne 1) {
            throw "Spike evidence missing unique scenario $name"
        }
        $scenario = $matches[0]
        if (@($scenario.violations).Count -ne 0) {
            throw "Spike evidence scenario violation at $name"
        }
        if ($scenario.seed -ne 1 -or $scenario.substeps -ne 4 -or
                $scenario.ticks -ne $expectedTicks[$name] -or
                $scenario.fallback -cne '') {
            throw "Spike evidence scenario execution identity mismatch for $name"
        }
        if ([string]$scenario.final_hash -cne $expectedScenarioHashes[$name]) {
            throw "Spike evidence canonical scenario hash mismatch for $name"
        }
        Assert-NinhoProperty $scenario 'step_ms' "scenario/$name"
        $stepValues = @()
        foreach ($stepName in @('min', 'p50', 'p95', 'max')) {
            Assert-NinhoProperty $scenario.step_ms $stepName "scenario/$name/step_ms"
            $stepValue = [double]$scenario.step_ms.$stepName
            if ([double]::IsNaN($stepValue) -or [double]::IsInfinity($stepValue) -or
                    $stepValue -lt 0) {
                throw "Spike evidence invalid step_ms value for $name/$stepName"
            }
            $stepValues += $stepValue
        }
        if ($stepValues[0] -gt $stepValues[1] -or
                $stepValues[1] -gt $stepValues[2] -or
                $stepValues[2] -gt $stepValues[3]) {
            throw "Spike evidence step_ms ordering mismatch for $name"
        }
        $limits = @($scenario.limits)
        if ($limits.Count -ne $expectedLimits[$name].Count) {
            throw "Spike evidence declared limit count mismatch for $name"
        }
        foreach ($expectedLimit in $expectedLimits[$name]) {
            $limit = @($limits | Where-Object name -CEQ $expectedLimit.name)
            if ($limit.Count -ne 1 -or
                    $limit[0].comparison -cne $expectedLimit.comparison -or
                    [double]$limit[0].value -ne [double]$expectedLimit.value -or
                    $limit[0].unit -cne $expectedLimit.unit) {
                throw "Spike evidence declared limit mismatch for $name/$($expectedLimit.name)"
            }
        }
        $fixtureNames = @('dynamic_body_count', 'shape_count', 'joint_count')
        for ($index = 0; $index -lt $fixtureNames.Count; ++$index) {
            Assert-NinhoProperty $scenario $fixtureNames[$index] "scenario/$name"
            if ($scenario.($fixtureNames[$index]) -ne $expectedFixtures[$name][$index]) {
                throw "Spike evidence fixture topology mismatch for $name/$($fixtureNames[$index])"
            }
        }
        $peakNames = @(
            'peak_body_count', 'peak_shape_count', 'peak_joint_count',
            'peak_awake_count', 'peak_contact_count')
        for ($index = 0; $index -lt $peakNames.Count; ++$index) {
            Assert-NinhoProperty $scenario $peakNames[$index] "scenario/$name"
            if ($scenario.($peakNames[$index]) -ne $expectedTopologies[$name][$index]) {
                throw "Spike evidence topology mismatch for $name/$($peakNames[$index])"
            }
        }
        switch ($name) {
            'radial_fall' {
                if ($scenario.surface_separation -lt -0.02 -or
                        $scenario.surface_separation -gt 0.03 -or
                        $scenario.final_linear_speed -ge 0.05 -or
                        $scenario.final_angular_speed -ge 0.1 -or
                        $scenario.max_energy_growth_ratio -gt 0.02) {
                    throw 'Spike evidence radial_fall normative metric is outside limits'
                }
            }
            'projectile_pile' {
                if ($scenario.contact_before_pile_exit -ne $true -or
                        $scenario.ccd_primary_pass_count -ne 20 -or
                        $scenario.ccd_fallback_pass_count -ne 0 -or
                        $scenario.projectile_speed -ne 35) {
                    throw 'Spike evidence projectile_pile normative metric mismatch'
                }
            }
            'radial_pile' {
                if ($scenario.sleep_ratio -lt 0.9 -or
                        $scenario.p95_linear_speed -ge 0.05 -or
                        $scenario.p95_angular_speed -ge 0.1 -or
                        $scenario.max_penetration -ge 0.02 -or
                        $scenario.max_energy_growth_ratio -gt 0.02 -or
                        $scenario.minimum_density -ne 480 -or
                        $scenario.maximum_density -ne 520) {
                    throw 'Spike evidence radial_pile normative metric is outside limits'
                }
            }
            'mass_ratio' {
                if ($scenario.sleep_ratio -lt 0.8 -or
                        $scenario.p95_linear_speed -ge 0.1 -or
                        $scenario.p95_angular_speed -ge 0.2 -or
                        $scenario.max_penetration -ge 0.025 -or
                        $scenario.minimum_density -ne 85 -or
                        $scenario.maximum_density -ne 3400) {
                    throw 'Spike evidence mass_ratio normative metric is outside limits'
                }
            }
            'stress' {
                if ($scenario.warmup_ticks -ne 300 -or
                        $scenario.measurement_ticks -ne 1200 -or
                        $scenario.allocator_warmup_cycles -ne 10 -or
                        $scenario.stress_cycles -ne 10) {
                    throw 'Spike evidence stress protocol mismatch'
                }
            }
        }

        $metricNames = @($scenario.metrics | ForEach-Object { $_.name })
        $metricNameDifference = @(Compare-Object `
            -ReferenceObject @($expectedMetricNames[$name] | Sort-Object) `
            -DifferenceObject @($metricNames | Sort-Object) `
            -CaseSensitive)
        if ($metricNameDifference.Count -ne 0 -or
                @($metricNames | Sort-Object -Unique).Count -ne $metricNames.Count) {
            throw "Spike evidence scenario metric set mismatch at $name"
        }
        for ($metricIndex = 0; $metricIndex -lt $expectedMetricNames[$name].Count; ++$metricIndex) {
            $metricName = $expectedMetricNames[$name][$metricIndex]
            $metric = @($scenario.metrics | Where-Object name -CEQ $metricName)[0]
            $metricValue = [double]$metric.value
            if ($metric.unit -cne $expectedMetricUnits[$name][$metricIndex] -or
                    [double]::IsNaN($metricValue) -or
                    [double]::IsInfinity($metricValue)) {
                throw "Spike evidence scenario metric value or unit mismatch for $name/$metricName"
            }
        }
        foreach ($propertyName in @(
                'surface_separation', 'final_linear_speed', 'final_angular_speed',
                'p95_linear_speed', 'p95_angular_speed', 'sleep_ratio',
                'max_penetration', 'minimum_density', 'maximum_density',
                'energy_at_tick_600', 'max_energy_growth_ratio',
                'allocator_warmup_cycles')) {
            $metric = @($scenario.metrics | Where-Object name -CEQ $propertyName)
            if ($metric.Count -gt 1 -or
                    ($metric.Count -eq 1 -and
                     [double]$metric[0].value -ne [double]$scenario.$propertyName)) {
                throw "Spike evidence scenario metric/property mismatch for $name/$propertyName"
            }
        }
        switch ($name) {
            'radial_fall' {
                if ((Get-NinhoScenarioMetric $scenario 'energy_window') -ne 120 -or
                        (Get-NinhoScenarioMetric $scenario 'energy_epsilon') -ne 1e-9) {
                    throw 'Spike evidence radial_fall metric protocol mismatch'
                }
            }
            'projectile_pile' {
                if ((Get-NinhoScenarioMetric $scenario 'primary_seed_passes') -ne $scenario.ccd_primary_pass_count -or
                        (Get-NinhoScenarioMetric $scenario 'fallback_seed_passes') -ne $scenario.ccd_fallback_pass_count -or
                        (Get-NinhoScenarioMetric $scenario 'projectile_speed') -ne $scenario.projectile_speed -or
                        (Get-NinhoScenarioMetric $scenario 'primary_invalid_states') -ne 0 -or
                        (Get-NinhoScenarioMetric $scenario 'fallback_invalid_states') -ne 0 -or
                        (Get-NinhoScenarioMetric $scenario 'fallback_seed_evaluated') -ne 0) {
                    throw 'Spike evidence projectile metric/property mismatch'
                }
            }
            'radial_pile' {
                $platform = Get-NinhoScenarioMetric $scenario 'max_platform_penetration'
                $pair = Get-NinhoScenarioMetric $scenario 'max_pair_penetration'
                if ([math]::Abs(
                        [math]::Max($platform, $pair) - [double]$scenario.max_penetration) -gt 1e-12) {
                    throw "Spike evidence penetration metric mismatch for $name"
                }
            }
            'mass_ratio' {
                $platform = Get-NinhoScenarioMetric $scenario 'max_platform_penetration'
                $pair = Get-NinhoScenarioMetric $scenario 'max_pair_penetration'
                if ([math]::Abs(
                        [math]::Max($platform, $pair) - [double]$scenario.max_penetration) -gt 1e-12) {
                    throw "Spike evidence penetration metric mismatch for $name"
                }
            }
            'stress' {
                if ((Get-NinhoScenarioMetric $scenario 'executed_substeps') -ne 4 -or
                        (Get-NinhoScenarioMetric $scenario 'allocator_warmup_cycles') -ne 10 -or
                        (Get-NinhoScenarioMetric $scenario 'completed_cycles') -ne 10 -or
                        (Get-NinhoScenarioMetric $scenario 'private_commit_diagnostic_growth_target') -ne 0.05 -or
                        (Get-NinhoScenarioMetric $scenario 'private_commit_diagnostic_trimmed_span_target') -ne 0.05 -or
                        (Get-NinhoScenarioMetric $scenario 'working_set_diagnostic_growth_target') -ne 0.05 -or
                        (Get-NinhoScenarioMetric $scenario 'working_set_diagnostic_trimmed_span_target') -ne 0.05) {
                    throw 'Spike evidence stress metric protocol mismatch'
                }
            }
        }
        $hashes = @($scenario.hashes)
        $observations = @($scenario.repeat_observations)
        if ($hashes.Count -ne 2 -or $observations.Count -ne 2 -or
                $hashes[0] -ne $hashes[1] -or $scenario.final_hash -ne $hashes[0]) {
            throw "Spike evidence determinism hash mismatch for $name"
        }
        for ($index = 0; $index -lt 2; ++$index) {
            $observation = $observations[$index]
            if ($observation.repeat -ne ($index + 1) -or
                    $observation.hash -ne $hashes[$index]) {
                throw "Spike evidence repeat observation mismatch for $name"
            }
            foreach ($peakName in $peakNames) {
                Assert-NinhoProperty $observation $peakName "scenario/$name/repeat/$($index + 1)"
                if ($observation.$peakName -ne $scenario.$peakName) {
                    throw "Spike evidence repeat topology mismatch for $name/$peakName"
                }
            }
            if ($observation.PSObject.Properties.Name -notcontains 'memory' -or
                    $null -eq $observation.memory) {
                throw "Spike evidence memory observation missing at scenario/$name/repeat/$($index + 1)"
            }
            Assert-NinhoMemoryObservation `
                -Memory $observation.memory `
                -Context "scenario/$name/repeat/$($index + 1)/memory"
            Assert-NinhoAllocator $observation.box3d_allocator "scenario/$name/repeat/$($index + 1)"
            $expectedCrt = $ExpectedBuildType -eq 'Debug' -and $name -eq 'stress'
            Assert-NinhoCrt $observation.crt $expectedCrt "scenario/$name/repeat/$($index + 1)/crt"
            Assert-NinhoAllocatorSamples `
                $observation.box3d_allocator ($name -eq 'stress') `
                "scenario/$name/repeat/$($index + 1)"
        }
        if ((ConvertTo-Json $scenario.memory -Depth 100 -Compress) -cne
                (ConvertTo-Json $observations[0].memory -Depth 100 -Compress)) {
            throw "Spike evidence scenario memory differs from first repeat for $name"
        }
        $scenarioWarnings = @($scenario.warnings)
        if ($name -eq 'stress') {
            if ($scenarioWarnings.Count -ne 1 -or
                    $scenarioWarnings[0].code -cne 'private_commit_budget_unqualified' -or
                    $scenarioWarnings[0].message -cne 'PrivateUsage is diagnostic in the foundation; qualify the 5% budget in a packaged Release build on reference hardware') {
                throw 'Spike evidence stress warning contract mismatch'
            }
            $assessmentDetail = @($scenarioWarnings[0].details | Where-Object name -CEQ 'assessment')
            $scopeDetail = @($scenarioWarnings[0].details | Where-Object name -CEQ 'budget_scope')
            if (@($scenarioWarnings[0].details).Count -ne 2 -or
                    $assessmentDetail.Count -ne 1 -or
                    $assessmentDetail[0].value -cne $observations[0].memory.assessment_status -or
                    $scopeDetail.Count -ne 1 -or
                    $scopeDetail[0].value -cne 'future_packaged_reference_hardware') {
                throw 'Spike evidence stress warning contract mismatch'
            }
        } elseif ($scenarioWarnings.Count -ne 0) {
            throw "Spike evidence unexpected scenario warning for $name"
        }
        if ($name -eq 'stress') {
            $memory = $observations[0].memory
            $metricMemoryMap = @{
                private_commit_baseline_median_bytes = [double]$memory.private_commit.baseline_median_bytes
                private_commit_final_median_bytes = [double]$memory.private_commit.final_median_bytes
                private_commit_growth_ratio = [double]$memory.private_commit.growth_ratio
                private_commit_instant_growth_ratio = [double]$memory.private_commit.instant_growth_ratio
                private_commit_warmup_trimmed_span_ratio = [double]$memory.private_commit.warmup_trimmed_span_ratio
                private_commit_measured_trimmed_span_ratio = [double]$memory.private_commit.measured_trimmed_span_ratio
                working_set_baseline_bytes = [double]$memory.working_set.baseline_last_bytes
                working_set_baseline_low_bytes = [double]$memory.working_set.baseline_full_min_bytes
                working_set_peak_bytes = [double]$memory.working_set.peak_bytes
                working_set_final_bytes = [double]$memory.working_set.final_last_bytes
                working_set_final_low_bytes = [double]$memory.working_set.final_full_min_bytes
                working_set_growth_ratio = [double]$memory.working_set.growth_ratio
                working_set_instant_growth_ratio = [double]$memory.working_set.instant_growth_ratio
            }
            foreach ($metricName in $metricMemoryMap.Keys) {
                $actualMetric = Get-NinhoScenarioMetric $scenario $metricName
                if ([math]::Abs($actualMetric - $metricMemoryMap[$metricName]) -gt 1e-12) {
                    throw "Spike evidence stress metric/memory mismatch for $metricName"
                }
            }
        }
        Assert-NinhoAllocator $scenario.box3d_allocator "scenario/$name"
        Assert-NinhoAllocatorSamples $scenario.box3d_allocator ($name -eq 'stress') "scenario/$name"
        $expectedScenarioCrt = $ExpectedBuildType -eq 'Debug' -and $name -eq 'stress'
        Assert-NinhoCrt $scenario.crt $expectedScenarioCrt "scenario/$name/crt"
    }

    $expectedCapabilities = @(
        'ccd_dynamic_dynamic', 'shape_cast_overlap', 'contact_hit_events',
        'joint_force_torque', 'hulls_compounds', 'radial_sleep',
        'batch_lifecycle', 'upstream_replay')
    $capabilityScenario = ($scenarios | Where-Object name -CEQ 'capability_matrix')[0]
    $topLevelMatrixJson = @($Document.matrix) | ConvertTo-Json -Depth 100 -Compress
    $scenarioMatrixJson = @($capabilityScenario.matrix) | ConvertTo-Json -Depth 100 -Compress
    if ($topLevelMatrixJson -cne $scenarioMatrixJson) {
        throw 'Spike evidence duplicate capability matrices diverge'
    }
    $expectedCapabilityTopologies = [ordered]@{
        ccd_dynamic_dynamic = @(123, 123, 0, 121, 221)
        shape_cast_overlap = @(1, 1, 0, 0, 0)
        contact_hit_events = @(2, 2, 0, 2, 1)
        joint_force_torque = @(2, 2, 1, 1, 1)
        hulls_compounds = @(2, 9, 0, 1, 8)
        radial_sleep = @(81, 81, 0, 80, 204)
        batch_lifecycle = @(1, 1, 0, 1, 0)
        upstream_replay = @(1, 1, 0, 1, 0)
    }
    $expectedCapabilityValueNames = @{
        ccd_dynamic_dynamic = @('primary_passes','fallback_passes','primary_speed','primary_substeps','fallback_speed','fallback_substeps','fallback_evaluated','primary_invalid_states','fallback_invalid_states')
        shape_cast_overlap = @('cast_distance','first_handle_index','first_handle_matches_overlap','fraction')
        contact_hit_events = @('approach_speed','effective_mass','derived_energy','normal_length','material_a','material_b','unique_pairs','substeps')
        joint_force_torque = @('monotonic_tolerance','maximum_force','rupture_threshold','maximum_deformation','deformation_threshold','consecutive_deformation_ticks')
        hulls_compounds = @('hull_count','expected_mass','mass','bounds_lower_x','bounds_lower_y','bounds_lower_z','bounds_upper_x','bounds_upper_y','bounds_upper_z','bounds_tolerance','contacted','state_valid','box3d_allocator_baseline_bytes','box3d_allocator_final_bytes')
        radial_sleep = @('sleep_ratio','p95_linear_speed','p95_angular_speed','max_penetration','energy_growth')
        batch_lifecycle = @('generation_cycles','invalid_handles','private_commit_available','private_commit_growth','private_commit_baseline_bytes','private_commit_final_bytes','working_set_baseline_bytes','working_set_final_bytes','crt_normal_count_delta','crt_normal_bytes_delta','crt_client_count_delta','crt_client_bytes_delta','box3d_allocator_baseline_bytes','box3d_allocator_final_bytes')
        upstream_replay = @('saved','loaded','validated','recording_bytes','temporary_file_removed','box3d_allocator_baseline_bytes','box3d_allocator_final_bytes')
    }
    $expectedCapabilityValueUnits = @{
        ccd_dynamic_dynamic = @('seeds','seeds','m/s','count','m/s','count','bool','count','count')
        shape_cast_overlap = @('m','index','bool','ratio')
        contact_hit_events = @('m/s','kg','J','ratio','id','id','count','count')
        joint_force_torque = @('N','N','N','m','m','ticks')
        hulls_compounds = @('count','kg','kg','m','m','m','m','m','m','m','bool','bool','bytes','bytes')
        radial_sleep = @('ratio','m/s','rad/s','m','ratio')
        batch_lifecycle = @('count','count','bool','ratio','bytes','bytes','bytes','bytes','count','bytes','count','bytes','bytes','bytes')
        upstream_replay = @('bool','bool','bool','bytes','bool','bytes','bytes')
    }
    $expectedCapabilityFixtureHashes = @{
        ccd_dynamic_dynamic = @('1431096453509785832','5466060815846980033','8526278817898170285','2675255426310783444','10494942409035688541','7390650343477109358','14725012844580521641','14001126938762718496','2400058324701230849','10732887959698602526','5915366692680379584','14334879908416984375','6577404406665493359','16513243213841685896','9721495563768596328','13937513265211812359','9628435969084559192','18192646134465658732','18426867884952182109','11807578939564631579')
        shape_cast_overlap = @('8388802905423810155')
        contact_hit_events = @('10155543446163919611')
        joint_force_torque = @('14239377403397705414')
        hulls_compounds = @('7710058609812386530')
        radial_sleep = @('10363635776067367757')
        batch_lifecycle = @('43224550866945')
        upstream_replay = @('13184605773762244167')
    }
    foreach ($matrix in @(@($Document.matrix), @($capabilityScenario.matrix))) {
        if ($matrix.Count -ne 8) {
            throw 'Spike evidence capability matrix row count mismatch'
        }
        foreach ($capability in $expectedCapabilities) {
            $rows = @($matrix | Where-Object capability -CEQ $capability)
            if ($rows.Count -ne 1 -or $rows[0].status -cne 'pass' -or
                    $rows[0].functional_status -cne 'pass') {
                throw "Spike evidence capability matrix failure for $capability"
            }
            for ($index = 0; $index -lt $peakNames.Count; ++$index) {
                $peakName = $peakNames[$index]
                Assert-NinhoProperty $rows[0] $peakName "matrix/$capability"
                if ($rows[0].$peakName -ne $expectedCapabilityTopologies[$capability][$index]) {
                    throw "Spike evidence capability topology mismatch for $capability/$peakName"
                }
            }
            $row = $rows[0]
            if (@($row.fixture_hashes).Count -eq 0) {
                throw "Spike evidence capability fixture hash missing for $capability"
            }
            $actualFixtureHashes = @($row.fixture_hashes | ForEach-Object { [string]$_ })
            if ((ConvertTo-Json $actualFixtureHashes -Compress) -cne
                    (ConvertTo-Json @($expectedCapabilityFixtureHashes[$capability]) -Compress)) {
                throw "Spike evidence canonical fixture hashes mismatch for $capability"
            }
            if ($null -ne $row.fallback -or $null -ne $row.functional_fallback) {
                throw "Spike evidence unexpected capability fallback for $capability"
            }
            $valueNames = @($row.values | ForEach-Object { $_.name })
            $valueNameDifference = @(Compare-Object `
                -ReferenceObject @($expectedCapabilityValueNames[$capability] | Sort-Object) `
                -DifferenceObject @($valueNames | Sort-Object) `
                -CaseSensitive)
            if ($valueNameDifference.Count -ne 0 -or
                    @($valueNames | Sort-Object -Unique).Count -ne $valueNames.Count) {
                throw "Spike evidence capability value set mismatch for $capability"
            }
            for ($valueIndex = 0; $valueIndex -lt $expectedCapabilityValueNames[$capability].Count; ++$valueIndex) {
                $valueName = $expectedCapabilityValueNames[$capability][$valueIndex]
                $valueEntry = @($row.values | Where-Object name -CEQ $valueName)[0]
                $numericValue = [double]$valueEntry.value
                if ($valueEntry.unit -cne $expectedCapabilityValueUnits[$capability][$valueIndex] -or
                        [double]::IsNaN($numericValue) -or
                        [double]::IsInfinity($numericValue)) {
                    throw "Spike evidence capability value or unit mismatch for $capability/$valueName"
                }
            }
            switch ($capability) {
                'ccd_dynamic_dynamic' {
                    if ((Get-NinhoCapabilityValue $row 'primary_passes') -ne 20 -or
                            (Get-NinhoCapabilityValue $row 'fallback_passes') -ne 0 -or
                            (Get-NinhoCapabilityValue $row 'primary_speed') -ne 35 -or
                            (Get-NinhoCapabilityValue $row 'primary_substeps') -ne 4 -or
                            (Get-NinhoCapabilityValue $row 'fallback_speed') -ne 30 -or
                            (Get-NinhoCapabilityValue $row 'fallback_substeps') -ne 6 -or
                            (Get-NinhoCapabilityValue $row 'fallback_evaluated') -ne 0 -or
                            (Get-NinhoCapabilityValue $row 'primary_invalid_states') -ne 0 -or
                            (Get-NinhoCapabilityValue $row 'fallback_invalid_states') -ne 0) {
                        throw 'Spike evidence CCD capability proof mismatch'
                    }
                }
                'shape_cast_overlap' {
                    $fraction = Get-NinhoCapabilityValue $row 'fraction'
                    if ((Get-NinhoCapabilityValue $row 'cast_distance') -ne 3 -or
                            (Get-NinhoCapabilityValue $row 'first_handle_index') -ne 1 -or
                            (Get-NinhoCapabilityValue $row 'first_handle_matches_overlap') -ne 1 -or
                            $fraction -lt 0 -or $fraction -gt 1) {
                        throw 'Spike evidence shape query capability proof mismatch'
                    }
                }
                'contact_hit_events' {
                    if ((Get-NinhoCapabilityValue $row 'approach_speed') -le 0 -or
                            (Get-NinhoCapabilityValue $row 'effective_mass') -le 0 -or
                            (Get-NinhoCapabilityValue $row 'derived_energy') -le 0 -or
                            (Get-NinhoCapabilityValue $row 'normal_length') -lt 0.9 -or
                            (Get-NinhoCapabilityValue $row 'material_a') -ne 111 -or
                            (Get-NinhoCapabilityValue $row 'material_b') -ne 222 -or
                            (Get-NinhoCapabilityValue $row 'unique_pairs') -lt 1 -or
                            (Get-NinhoCapabilityValue $row 'substeps') -ne 6) {
                        throw 'Spike evidence contact capability proof mismatch'
                    }
                }
                'joint_force_torque' {
                    $maximumDeformation = Get-NinhoCapabilityValue $row 'maximum_deformation'
                    $consecutiveTicks = Get-NinhoCapabilityValue $row 'consecutive_deformation_ticks'
                    if ((Get-NinhoCapabilityValue $row 'maximum_force') -le 10000 -or
                            (Get-NinhoCapabilityValue $row 'rupture_threshold') -ne 10000 -or
                            (Get-NinhoCapabilityValue $row 'monotonic_tolerance') -ne 50 -or
                            (Get-NinhoCapabilityValue $row 'deformation_threshold') -ne 0.01 -or
                            $maximumDeformation -lt 0 -or
                            $consecutiveTicks -notin 0, 2) {
                        throw 'Spike evidence joint capability proof mismatch'
                    }
                }
                'hulls_compounds' {
                    $mass = Get-NinhoCapabilityValue $row 'mass'
                    $expectedMass = Get-NinhoCapabilityValue $row 'expected_mass'
                    $tolerance = Get-NinhoCapabilityValue $row 'bounds_tolerance'
                    if ((Get-NinhoCapabilityValue $row 'hull_count') -ne 8 -or
                            $expectedMass -ne 42.666666666666664 -or
                            $tolerance -ne 1e-4 -or
                            [math]::Abs($mass - $expectedMass) -gt 1e-4 -or
                            [math]::Abs((Get-NinhoCapabilityValue $row 'bounds_lower_x') + 1.62) -gt $tolerance -or
                            [math]::Abs((Get-NinhoCapabilityValue $row 'bounds_lower_y') - 2.78) -gt $tolerance -or
                            [math]::Abs((Get-NinhoCapabilityValue $row 'bounds_lower_z') + 0.22) -gt $tolerance -or
                            [math]::Abs((Get-NinhoCapabilityValue $row 'bounds_upper_x') - 1.62) -gt $tolerance -or
                            [math]::Abs((Get-NinhoCapabilityValue $row 'bounds_upper_y') - 3.22) -gt $tolerance -or
                            [math]::Abs((Get-NinhoCapabilityValue $row 'bounds_upper_z') - 0.22) -gt $tolerance -or
                            (Get-NinhoCapabilityValue $row 'contacted') -ne 1 -or
                            (Get-NinhoCapabilityValue $row 'state_valid') -ne 1 -or
                            (Get-NinhoCapabilityValue $row 'box3d_allocator_baseline_bytes') -ne 0 -or
                            (Get-NinhoCapabilityValue $row 'box3d_allocator_final_bytes') -ne 0) {
                        throw 'Spike evidence hull capability proof mismatch'
                    }
                }
                'radial_sleep' {
                    if ((Get-NinhoCapabilityValue $row 'sleep_ratio') -lt 0.9 -or
                            (Get-NinhoCapabilityValue $row 'p95_linear_speed') -ge 0.05 -or
                            (Get-NinhoCapabilityValue $row 'p95_angular_speed') -ge 0.1 -or
                            (Get-NinhoCapabilityValue $row 'max_penetration') -ge 0.02 -or
                            (Get-NinhoCapabilityValue $row 'energy_growth') -gt 0.02) {
                        throw 'Spike evidence radial sleep capability proof mismatch'
                    }
                }
                'batch_lifecycle' {
                    $privateAvailable = Get-NinhoCapabilityValue $row 'private_commit_available'
                    $privateGrowth = Get-NinhoCapabilityValue $row 'private_commit_growth'
                    $privateBaseline = Get-NinhoCapabilityValue $row 'private_commit_baseline_bytes'
                    $privateFinal = Get-NinhoCapabilityValue $row 'private_commit_final_bytes'
                    $workingBaseline = Get-NinhoCapabilityValue $row 'working_set_baseline_bytes'
                    $workingFinal = Get-NinhoCapabilityValue $row 'working_set_final_bytes'
                    $derivedPrivateGrowth = if ($privateBaseline -gt 0) {
                        [math]::Max(0.0, ($privateFinal - $privateBaseline) / $privateBaseline)
                    } else { 0.0 }
                    if ((Get-NinhoCapabilityValue $row 'generation_cycles') -ne 10000 -or
                            (Get-NinhoCapabilityValue $row 'invalid_handles') -ne 0 -or
                            $privateAvailable -ne 1 -or
                            $privateBaseline -le 0 -or $privateFinal -le 0 -or
                            [math]::Abs($privateGrowth - $derivedPrivateGrowth) -gt 1e-12 -or
                            $workingBaseline -le 0 -or $workingFinal -le 0 -or
                            (Get-NinhoCapabilityValue $row 'crt_normal_count_delta') -ne 0 -or
                            (Get-NinhoCapabilityValue $row 'crt_normal_bytes_delta') -ne 0 -or
                            (Get-NinhoCapabilityValue $row 'crt_client_count_delta') -ne 0 -or
                            (Get-NinhoCapabilityValue $row 'crt_client_bytes_delta') -ne 0 -or
                            (Get-NinhoCapabilityValue $row 'box3d_allocator_baseline_bytes') -ne 0 -or
                            (Get-NinhoCapabilityValue $row 'box3d_allocator_final_bytes') -ne 0) {
                        throw 'Spike evidence lifecycle capability proof mismatch'
                    }
                }
                'upstream_replay' {
                    if ((Get-NinhoCapabilityValue $row 'saved') -ne 1 -or
                            (Get-NinhoCapabilityValue $row 'loaded') -ne 1 -or
                            (Get-NinhoCapabilityValue $row 'validated') -ne 1 -or
                            (Get-NinhoCapabilityValue $row 'recording_bytes') -le 0 -or
                            (Get-NinhoCapabilityValue $row 'temporary_file_removed') -ne 1 -or
                            (Get-NinhoCapabilityValue $row 'box3d_allocator_baseline_bytes') -ne 0 -or
                            (Get-NinhoCapabilityValue $row 'box3d_allocator_final_bytes') -ne 0) {
                        throw 'Spike evidence replay capability proof mismatch'
                    }
                }
            }
        }
        foreach ($peakName in $peakNames) {
            $maximum = ($matrix | Measure-Object -Property $peakName -Maximum).Maximum
            if ($maximum -ne (($scenarios | Where-Object name -CEQ 'capability_matrix')[0]).$peakName) {
                throw "Spike evidence capability aggregate mismatch for $peakName"
            }
        }
    }
}

Export-ModuleMember -Function Assert-NinhoSpikeEvidenceDocument
