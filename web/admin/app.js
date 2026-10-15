document.addEventListener('DOMContentLoaded', () => {
  const nativeFetch = window.fetch.bind(window);
  window.fetch = (input, init = {}) => {
    let token = sessionStorage.getItem('eloAdminToken');
    if (!token) {
      token = window.prompt('Token administrativo ELO:') || '';
      if (token) sessionStorage.setItem('eloAdminToken', token);
    }
    const headers = new Headers(init.headers || {});
    // Only send the admin token to same-origin /api/* endpoints
    let requestUrl = null;
    try {
      requestUrl = typeof input === 'string' ? new URL(input, window.location.href) : new URL(input.url, window.location.href);
    } catch (_) {
      // Fallback
    }
    const isSameOriginApi = requestUrl && (requestUrl.origin === window.location.origin) && requestUrl.pathname.startsWith('/api/');
    if (isSameOriginApi && token) {
      headers.set('Authorization', `Bearer ${token}`);
    }
    return nativeFetch(input, { ...init, headers }).then(response => {
      if (response.status === 401 && isSameOriginApi) sessionStorage.removeItem('eloAdminToken');
      return response;
    });
  };

  function escapeHtml(value) {
    if (value === null || value === undefined) return '';
    return String(value)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#39;');
  }

  let catalogData = { atoms: [], relations: [], recipes: [] };
  let currentStatus = null;
  let packageAtomIds = new Set();
  let packagePlansData = [];
  let selectedPackageId = '';

  // Tab switching
  const tabButtons = document.querySelectorAll('.tab-btn');
  const tabPanes = document.querySelectorAll('.tab-pane');

  tabButtons.forEach(btn => {
    btn.addEventListener('click', () => {
      tabButtons.forEach(b => b.classList.remove('active'));
      tabPanes.forEach(p => p.classList.remove('active'));

      btn.classList.add('active');
      const target = document.getElementById(btn.getAttribute('data-tab'));
      if (target) {
        target.classList.add('active');
        if (target.id === 'tab-media') fetchMedia();
        if (target.id === 'tab-bundles') fetchManifest().then(() => fetchPackagePlans());
        if (target.id === 'tab-analytics') fetchAnalytics();
      }
    });
  });

  // Notice helper
  const noticeBanner = document.getElementById('notice-banner');
  const noticeTitle = document.getElementById('notice-title');
  const noticeDesc = document.getElementById('notice-desc');
  const noticeClose = document.getElementById('notice-close');

  function showNotice(title, desc, isError = false) {
    noticeBanner.classList.remove('hidden', 'notice-success', 'notice-error');
    noticeBanner.classList.add(isError ? 'notice-error' : 'notice-success');
    noticeTitle.textContent = title;
    noticeDesc.textContent = desc;
  }

  noticeClose.addEventListener('click', () => {
    noticeBanner.classList.add('hidden');
  });

  // Lifecycle visual updates
  function updateLifecycleUI(isDraft) {
    const stepDraft = document.getElementById('step-draft');
    const stepActive = document.getElementById('step-active');
    const lifecycleName = document.getElementById('lifecycle-current-name');
    const badge = document.getElementById('bundle-badge');

    if (isDraft) {
      stepDraft.className = 'step-item step-warning';
      stepActive.className = 'step-item';
      lifecycleName.textContent = 'DRAFT (Alterações locais não publicadas)';
      lifecycleName.style.color = 'var(--accent-amber)';
      badge.textContent = 'RASCUNHO EDITORIAL';
      badge.style.color = 'var(--accent-amber)';
      badge.style.borderColor = 'rgba(245, 158, 11, 0.4)';
      badge.style.background = 'rgba(245, 158, 11, 0.15)';
    } else {
      stepDraft.className = 'step-item step-completed';
      stepActive.className = 'step-item step-current';
      lifecycleName.textContent = 'ACTIVE (No Totem)';
      lifecycleName.style.color = 'var(--accent-emerald)';
      badge.textContent = 'APLICAÇÃO ATIVA';
      badge.style.color = 'var(--accent-emerald)';
      badge.style.borderColor = 'rgba(16, 185, 129, 0.3)';
      badge.style.background = 'rgba(16, 185, 129, 0.15)';
    }
  }

  // State fetchers
  async function fetchStatus() {
    try {
      const res = await fetch('/api/status');
      if (!res.ok) throw new Error('Status HTTP error: ' + res.status);
      const data = await res.json();
      currentStatus = data;

      // Kiosk Status Pill
      const kioskPill = document.getElementById('kiosk-status-pill');
      const kioskText = document.getElementById('kiosk-status-text');
      kioskPill.classList.remove('status-checking', 'status-online', 'status-offline');
      if (data.kiosk_online) {
        kioskPill.classList.add('status-online');
        kioskText.textContent = 'Kiosk: ONLINE (IPC OK)';
      } else {
        kioskPill.classList.add('status-offline');
        kioskText.textContent = 'Kiosk: OFFLINE / PARADO';
      }

      // Kiosk Power Control Buttons Visibility
      const btnStart = document.getElementById('btn-kiosk-start');
      const btnRestart = document.getElementById('btn-kiosk-restart');
      const btnStop = document.getElementById('btn-kiosk-stop');
      if (btnStart && btnRestart && btnStop) {
        if (data.kiosk_online) {
          btnStart.style.display = 'none';
          btnRestart.style.display = 'inline-flex';
          btnStop.style.display = 'inline-flex';
        } else {
          btnStart.style.display = 'inline-flex';
          btnRestart.style.display = 'none';
          btnStop.style.display = 'none';
        }
      }

      // Live totem active atom indicator
      const livePill = document.getElementById('kiosk-live-pill');
      const liveText = document.getElementById('kiosk-live-text');
      if (livePill) {
        if (data.kiosk_online && (data.kiosk_active_atom_title || data.kiosk_active_atom_id)) {
          livePill.style.display = 'inline-flex';
          const displayTitle = data.kiosk_active_atom_title || data.kiosk_active_atom_id;
          liveText.textContent = `Totem: ${displayTitle}`;
        } else {
          livePill.style.display = 'none';
        }
      }

      // Socket path in IPC tab
      document.getElementById('ipc-socket-path').textContent = data.control_socket || '/run/elo/control.sock';
      const connState = document.getElementById('ipc-conn-state');
      connState.textContent = data.kiosk_online ? 'Conectado ao elo-kiosk (Online)' : 'Kiosk desligado ou offline';
      connState.className = data.kiosk_online ? 'badge-pill badge-green' : 'badge-pill';

      const btnIpcStart = document.getElementById('btn-ipc-start');
      const btnIpcRestart = document.getElementById('btn-ipc-restart');
      const btnIpcStop = document.getElementById('btn-ipc-stop');
      if (btnIpcStart && btnIpcRestart && btnIpcStop) {
        btnIpcStart.disabled = data.kiosk_online;
        btnIpcRestart.disabled = !data.kiosk_online;
        btnIpcStop.disabled = !data.kiosk_online;
      }

      // Active Bundle Card
      if (data.active_bundle) {
        document.getElementById('active-bundle-title').textContent = data.active_bundle.title || data.active_bundle.bundle_id;
        document.getElementById('active-bundle-desc').textContent = data.active_bundle.description || '';
        document.getElementById('active-bundle-hash').textContent = data.active_bundle.content_hash || 'Sem hash';
        document.getElementById('stat-version').textContent = 'v' + data.active_bundle.version;
        document.getElementById('stat-revision').textContent = '#' + data.active_bundle.curation_revision;
      } else {
        document.getElementById('active-bundle-title').textContent = 'Biodiversidade do Bioma Pampa & Campos Sulinos';
        document.getElementById('active-bundle-desc').textContent = 'Catálogo soberano local em modo Rascunho Editorial. Crie e ative uma aplicação para publicar a primeira versão.';
        document.getElementById('active-bundle-hash').textContent = 'Pronto para publicação determinística';
        document.getElementById('stat-version').textContent = 'v1.0.0';
        document.getElementById('stat-revision').textContent = '#1 (Draft)';
      }

      // Editorial State
      updateLifecycleUI(data.editorial_state === 'draft' || data.draft_modified);

      // Bundles table
      renderBundlesTable(data.bundles || [], data.active_bundle);
    } catch (err) {
      console.warn('Erro ao consultar status:', err);
    }
  }

  async function fetchCatalog() {
    try {
      const res = await fetch('/api/catalog');
      if (!res.ok) throw new Error('Catalog HTTP error: ' + res.status);
      const data = await res.json();
      catalogData = data;

      document.getElementById('stat-atoms').textContent = data.atoms ? data.atoms.length : 0;
      document.getElementById('stat-relations').textContent = data.relations ? data.relations.length : (data.atoms ? Math.round(data.atoms.length * 0.8) : 0);
      document.getElementById('stat-recipes').textContent = data.recipes ? data.recipes.length : 0;

      renderAtoms(data.atoms || []);
      renderRelations(data.relations || []);
      renderRecipes(data.recipes || []);
      renderPackageAtoms(data.atoms || []);
    } catch (err) {
      console.warn('Erro ao carregar catálogo:', err);
    }
  }

  function applyPackageManifest(m) {
      if (document.getElementById('pkg-title')) document.getElementById('pkg-title').value = m.title || '';
      if (document.getElementById('pkg-id')) document.getElementById('pkg-id').value = m.bundle_id || '';
      if (document.getElementById('pkg-version')) document.getElementById('pkg-version').value = m.version || '';
      if (document.getElementById('pkg-theme')) document.getElementById('pkg-theme').value = m.default_theme || '';
      if (document.getElementById('pkg-desc')) document.getElementById('pkg-desc').value = m.description || '';
      if (document.getElementById('pkg-domain')) document.getElementById('pkg-domain').value = m.domain_category || '';
      if (document.getElementById('pkg-audience')) document.getElementById('pkg-audience').value = m.target_audience || '';
      const initialAtoms = Array.isArray(m.atom_ids) && m.atom_ids.length > 0 ? m.atom_ids : (catalogData.atoms || []).map(a => a.content_id);
      packageAtomIds = new Set(initialAtoms);
      selectedPackageId = m.bundle_id || '';
      const heading = document.getElementById('pkg-editor-heading');
      if (heading) heading.textContent = m.title || 'Nova aplicação';
      const state = document.getElementById('package-badge-state');
      const isActive = currentStatus?.active_bundle?.bundle_id === selectedPackageId;
      if (state) {
        state.textContent = isActive ? 'ATIVO NO TOTEM' : 'RASCUNHO';
        state.className = isActive ? 'badge-pill badge-green' : 'badge-pill';
      }
      renderPackageAtoms(catalogData.atoms || []);
  }

  async function fetchManifest() {
    try {
      const res = await fetch('/api/manifest');
      if (!res.ok) return;
      const m = await res.json();
      applyPackageManifest(m);
    } catch (e) {
      console.warn('Erro ao carregar manifesto:', e);
    }
  }

  async function fetchPackagePlans(preferredId = '') {
    const select = document.getElementById('pkg-plan-select');
    if (!select) return;
    try {
      const res = await fetch('/api/packages');
      if (!res.ok) return;
      const data = await res.json();
      const currentId = preferredId || document.getElementById('pkg-id')?.value || '';
      const plans = data.packages || [];
      packagePlansData = plans;
      selectedPackageId = currentId;
      select.innerHTML = '<option value="">Nova aplicação</option>';
      plans.forEach(plan => {
        const option = document.createElement('option');
        option.value = plan.bundle_id;
        option.textContent = `${plan.title || plan.bundle_id} (v${plan.version || '0.1.0'})`;
        option.selected = plan.bundle_id === currentId;
        select.appendChild(option);
      });
      select.value = plans.some(plan => plan.bundle_id === currentId) ? currentId : '';

      const list = document.getElementById('pkg-plans-list');
      const count = document.getElementById('pkg-plans-count');
      if (count) count.textContent = plans.length;
      if (list) {
        list.innerHTML = plans.length ? '' : '<div class="empty-state"><strong>Nenhuma aplicação salva</strong><p>Crie a primeira aplicação e selecione seus conteúdos.</p></div>';
        plans.forEach(plan => {
          const isActive = !!plan.is_active;
          const item = document.createElement('article');
          item.className = `package-plan-item ${plan.bundle_id === currentId ? 'selected' : ''} ${isActive ? 'active' : ''}`;

          const titleRow = document.createElement('div');
          titleRow.className = 'package-plan-title-row';
          const titleSpan = document.createElement('span');
          titleSpan.className = 'package-plan-title';
          titleSpan.textContent = plan.title || plan.bundle_id;
          titleRow.appendChild(titleSpan);
          if (isActive) {
            const activeBadge = document.createElement('span');
            activeBadge.className = 'badge-pill badge-green';
            activeBadge.style.fontSize = '9px';
            activeBadge.textContent = 'ATIVO';
            titleRow.appendChild(activeBadge);
          }
          item.appendChild(titleRow);

          const metaDiv = document.createElement('div');
          metaDiv.className = 'package-plan-meta';
          metaDiv.textContent = `v${plan.version || '0.1.0'} · ${(plan.atom_ids || []).length} átomos · ${plan.is_published ? 'publicado' : 'não publicado'}`;
          metaDiv.appendChild(document.createElement('br'));
          metaDiv.appendChild(document.createTextNode(plan.bundle_id));
          item.appendChild(metaDiv);

          const actionsDiv = document.createElement('div');
          actionsDiv.className = 'package-plan-actions';
          actionsDiv.innerHTML = `
            <button type="button" class="btn btn-secondary btn-edit-package">Editar</button>
            <button type="button" class="btn ${isActive ? 'btn-danger-outline' : 'btn-primary'} btn-activate-package">${isActive ? 'Desativar' : 'Ativar'}</button>
            <button type="button" class="btn btn-danger-outline btn-delete-package" ${isActive ? 'disabled' : ''} title="Excluir plano">×</button>
          `;
          item.appendChild(actionsDiv);

          item.addEventListener('click', (event) => {
            if (!event.target.closest('button')) loadPackagePlan(plan.bundle_id);
          });
          item.querySelector('.btn-edit-package').addEventListener('click', () => loadPackagePlan(plan.bundle_id));
          item.querySelector('.btn-activate-package').addEventListener('click', () => isActive ? deactivatePackagePlan(plan) : activatePackagePlan(plan));
          item.querySelector('.btn-delete-package').addEventListener('click', () => deletePackagePlan(plan));
          list.appendChild(item);
        });
      }
    } catch (e) {
      console.warn('Erro ao listar aplicações:', e);
    }
  }

  async function activatePackagePlan(plan) {
    if (!plan || !plan.bundle_id) return;
    if (!confirm(`Publicar e ativar a aplicação "${plan.title || plan.bundle_id}" no Totem?`)) return;
    try {
      showNotice('Ativando aplicação…', `Validando e publicando ${plan.title || plan.bundle_id}.`);
      const publishResponse = await fetch(`/api/packages/${encodeURIComponent(plan.bundle_id)}/activate`, { method: 'POST' });
      const result = await publishResponse.json();
      if (!result.success) throw new Error(result.error || 'Falha ao publicar a aplicação');
      showNotice('Aplicação ativa', `${plan.title || plan.bundle_id} foi publicada e ativada no Totem.`);
      await fetchStatus();
      await fetchPackagePlans(plan.bundle_id);
    } catch (error) {
      showNotice('Falha ao ativar aplicação', error.message, true);
    }
  }

  async function deactivatePackagePlan(plan) {
    if (!plan || !plan.bundle_id) return;
    if (!confirm(`Desativar a aplicação "${plan.title || plan.bundle_id}" no Totem?\n\nEla continuará salva e suas versões permanecerão publicadas.`)) return;
    try {
      showNotice('Desativando aplicação…', plan.title || plan.bundle_id);
      const response = await fetch(`/api/packages/${encodeURIComponent(plan.bundle_id)}/deactivate`, { method: 'POST' });
      const result = await response.json();
      if (!response.ok || !result.success) throw new Error(result.error || 'Falha ao desativar aplicação');
      showNotice('Aplicação desativada', `${plan.title || plan.bundle_id} continua salva e publicada.`);
      await fetchStatus();
      await fetchPackagePlans(plan.bundle_id);
    } catch (error) {
      showNotice('Falha ao desativar aplicação', error.message, true);
    }
  }

  async function deletePackagePlan(plan) {
    if (!plan || plan.is_active || !confirm(`Excluir a aplicação "${plan.title || plan.bundle_id}"?\n\nVersões já publicadas permanecerão no histórico.`)) return;
    try {
      const response = await fetch(`/api/packages/${encodeURIComponent(plan.bundle_id)}`, { method: 'DELETE' });
      const result = await response.json();
      if (!response.ok || !result.success) throw new Error(result.error || 'Falha ao excluir aplicação');
      showNotice('Aplicação excluída', `${plan.title || plan.bundle_id} foi removida.`);
      hidePackageEditor();
      await fetchPackagePlans();
    } catch (error) {
      showNotice('Falha ao excluir aplicação', error.message, true);
    }
  }

  async function loadPackagePlan(id) {
    if (!id) return;
    try {
      const res = await fetch(`/api/packages/${encodeURIComponent(id)}`);
      if (!res.ok) throw new Error('Aplicação não encontrada');
      applyPackageManifest(await res.json());
      document.getElementById('pkg-id').disabled = true;
      showPackageEditor();
      await fetchPackagePlans(id);
    } catch (e) {
      showNotice('Erro ao carregar aplicação', e.message, true);
    }
  }

  function showPackageEditor() {
    document.getElementById('package-workspace')?.classList.remove('library-only');
    document.getElementById('package-editor-card')?.classList.remove('hidden');
    document.getElementById('package-atoms-card')?.classList.remove('hidden');
  }

  function hidePackageEditor() {
    document.getElementById('package-workspace')?.classList.add('library-only');
    document.getElementById('package-editor-card')?.classList.add('hidden');
    document.getElementById('package-atoms-card')?.classList.add('hidden');
  }

  document.getElementById('pkg-plan-select')?.addEventListener('change', (event) => loadPackagePlan(event.target.value));

  function renderPackageAtoms(atoms) {
    const list = document.getElementById('pkg-atoms-list');
    const badge = document.getElementById('pkg-atoms-count-badge');
    if (!list) return;
    list.innerHTML = '';
    const selectedCount = packageAtomIds.size;
    if (badge) badge.textContent = `${selectedCount} / ${atoms.length} Átomos`;

    if (atoms.length === 0) {
      list.innerHTML = '<p style="color:var(--text-dim); padding:10px;">Nenhum átomo cadastrado nesta aplicação.</p>';
      return;
    }

    const query = (document.getElementById('pkg-atoms-search')?.value || '').trim().toLocaleLowerCase('pt-BR');
    const visibleAtoms = query ? atoms.filter(a => `${a.title || ''} ${a.canonical_name || ''} ${a.content_id || ''} ${a.type || ''}`.toLocaleLowerCase('pt-BR').includes(query)) : atoms;
    visibleAtoms.forEach(a => {
      const item = document.createElement('div');
      item.className = 'package-atom-item';

      const check = document.createElement('input');
      check.type = 'checkbox';
      check.className = 'pkg-atom-check';
      check.dataset.id = a.content_id;
      check.checked = packageAtomIds.has(a.content_id);
      check.setAttribute('aria-label', `Incluir ${a.title || a.content_id} na aplicação`);

      const textWrap = document.createElement('div');
      const titleStrong = document.createElement('strong');
      titleStrong.style.cssText = 'color: var(--text-main); font-size: 0.9rem;';
      titleStrong.textContent = a.canonical_name || a.title;

      const subInfo = document.createElement('div');
      subInfo.style.cssText = 'color: var(--text-dim); font-size: 0.75rem;';
      subInfo.textContent = `${a.type_label || a.type} · `;
      const codeElem = document.createElement('code');
      codeElem.textContent = a.content_id;
      subInfo.appendChild(codeElem);

      textWrap.appendChild(titleStrong);
      textWrap.appendChild(subInfo);

      const typeBadge = document.createElement('span');
      typeBadge.className = 'badge-pill';
      typeBadge.style.fontSize = '10px';
      typeBadge.textContent = a.type;

      item.appendChild(check);
      item.appendChild(textWrap);
      item.appendChild(typeBadge);
      list.appendChild(item);
    });
    list.querySelectorAll('.pkg-atom-check').forEach(check => {
      check.addEventListener('change', () => {
        if (check.checked) packageAtomIds.add(check.dataset.id); else packageAtomIds.delete(check.dataset.id);
        if (badge) badge.textContent = `${packageAtomIds.size} / ${atoms.length} Átomos`;
      });
    });
  }

  document.getElementById('pkg-atoms-search')?.addEventListener('input', () => renderPackageAtoms(catalogData.atoms || []));

  document.getElementById('btn-new-package')?.addEventListener('click', () => {
    document.getElementById('form-package-meta')?.reset();
    document.getElementById('pkg-version').value = '0.1.0';
    document.getElementById('pkg-theme').value = 'default';
    packageAtomIds = new Set(catalogData.atoms.map(atom => atom.content_id));
    selectedPackageId = '';
    document.getElementById('pkg-id').disabled = false;
    document.getElementById('pkg-editor-heading').textContent = 'Nova aplicação';
    document.getElementById('package-badge-state').textContent = 'NOVO';
    renderPackageAtoms(catalogData.atoms || []);
    fetchPackagePlans('');
    showPackageEditor();
    document.getElementById('pkg-title')?.focus();
  });

  document.getElementById('btn-cancel-package')?.addEventListener('click', () => {
    hidePackageEditor();
    fetchPackagePlans(selectedPackageId);
  });



  async function fetchAnalytics() {
    try {
      const filter = document.getElementById('analytics-app-filter');
      if (filter && filter.options.length <= 1) {
        const applicationsResponse = await fetch('/api/packages');
        if (applicationsResponse.ok) {
          const applications = (await applicationsResponse.json()).packages || [];
          applications.forEach(application => {
            const option = document.createElement('option');
            option.value = application.bundle_id;
            option.textContent = application.title || application.bundle_id;
            filter.appendChild(option);
          });
        }
      }
      const applicationId = filter?.value || '';
      const endpoint = applicationId
        ? `/api/analytics/summary?application_id=${encodeURIComponent(applicationId)}`
        : '/api/analytics/summary';
      const res = await fetch(endpoint);
      if (!res.ok) return;
      const data = await res.json();

      if (document.getElementById('stat-an-sessions')) document.getElementById('stat-an-sessions').textContent = data.total_sessions || 0;
      if (document.getElementById('stat-an-dwell')) document.getElementById('stat-an-dwell').textContent = `${(data.average_dwell_time_seconds || 0).toFixed(1)}s`;
      if (document.getElementById('stat-an-views')) document.getElementById('stat-an-views').textContent = data.total_atom_views || 0;
      if (document.getElementById('stat-an-recipes')) document.getElementById('stat-an-recipes').textContent = data.total_recipe_completions || 0;

      // Render Dwell List
      const dwellList = document.getElementById('analytics-dwell-list');
      if (dwellList) {
        dwellList.innerHTML = '';
        const entries = Object.entries(data.atom_avg_dwell_seconds || {});
        if (entries.length === 0) {
          dwellList.innerHTML = '<p style="color:var(--text-dim); font-size:0.85rem;">Sem dados de permanência agregados ainda.</p>';
        } else {
          entries.forEach(([atom, avg]) => {
            const count = (data.atom_view_counts || {})[atom] || 1;
            const item = document.createElement('div');
            item.style.cssText = 'display:flex; justify-content:space-between; align-items:center; background:var(--bg-surface); padding:10px 14px; border-radius:var(--radius-md); border:1px solid var(--border-subtle);';
            const leftCol = document.createElement('div');
            const atomStrong = document.createElement('strong');
            atomStrong.style.cssText = 'color:var(--text-main); font-size:0.9rem;';
            atomStrong.textContent = atom;
            const countDiv = document.createElement('div');
            countDiv.style.cssText = 'color:var(--text-dim); font-size:0.75rem;';
            countDiv.textContent = `${count} visualizações agregadas`;
            leftCol.appendChild(atomStrong);
            leftCol.appendChild(countDiv);

            const badgeSpan = document.createElement('span');
            badgeSpan.className = 'badge-pill badge-green';
            badgeSpan.textContent = `${avg.toFixed(1)}s de contemplação média`;

            item.appendChild(leftCol);
            item.appendChild(badgeSpan);
            dwellList.appendChild(item);
          });
        }
      }

      // Render Flow Matrix
      const flowList = document.getElementById('analytics-flow-list');
      if (flowList) {
        flowList.innerHTML = '';
        const matrix = data.transition_matrix || {};
        const flows = [];
        for (const [src, targets] of Object.entries(matrix)) {
          for (const [tgt, count] of Object.entries(targets)) {
            flows.push({ src, tgt, count });
          }
        }

        if (flows.length === 0) {
          flowList.innerHTML = '<p style="color:var(--text-dim); font-size:0.85rem;">Sem dados de transição agregados ainda.</p>';
        } else {
          flows.forEach(f => {
            const item = document.createElement('div');
            item.style.cssText = 'display:flex; justify-content:space-between; align-items:center; background:var(--bg-surface); padding:10px 14px; border-radius:var(--radius-md); border:1px solid var(--border-subtle);';
            const flowWrap = document.createElement('div');
            flowWrap.style.cssText = 'display:flex; align-items:center; gap:8px; font-size:0.85rem;';
            const srcCode = document.createElement('code');
            srcCode.style.color = 'var(--text-main)';
            srcCode.textContent = f.src;
            const arrowSpan = document.createElement('span');
            arrowSpan.style.color = 'var(--accent-cyan)';
            arrowSpan.textContent = '➔';
            const tgtCode = document.createElement('code');
            tgtCode.style.color = 'var(--text-main)';
            tgtCode.textContent = f.tgt;
            flowWrap.appendChild(srcCode);
            flowWrap.appendChild(arrowSpan);
            flowWrap.appendChild(tgtCode);

            const countBadge = document.createElement('span');
            countBadge.className = 'badge-pill';
            countBadge.style.fontSize = '10px';
            countBadge.textContent = `${f.count} transições`;

            item.appendChild(flowWrap);
            item.appendChild(countBadge);
            flowList.appendChild(item);
          });
        }
      }
    } catch (e) {
      console.warn('Erro ao carregar analíticas:', e);
    }
  }

  function renderAtoms(atoms) {
    const container = document.getElementById('atoms-grid');
    container.innerHTML = '';

    if (atoms.length === 0) {
      container.innerHTML = '<p style="color:var(--text-dim); padding:20px;">Nenhum átomo de conteúdo no catálogo.</p>';
      return;
    }

    atoms.forEach(atom => {
      const card = document.createElement('div');
      card.className = 'atom-card';

      // Header
      const headerDiv = document.createElement('div');
      headerDiv.className = 'atom-header';

      const titleGroup = document.createElement('div');
      const titleH4 = document.createElement('h4');
      titleH4.className = 'atom-title';
      titleH4.textContent = atom.title || atom.canonical_name || '';

      const isLiveOnKiosk = currentStatus && currentStatus.kiosk_active_atom_id === atom.content_id;
      if (isLiveOnKiosk) {
        const liveBadge = document.createElement('span');
        liveBadge.className = 'badge-pill badge-green';
        liveBadge.style.cssText = 'font-size: 10px; margin-left: 6px;';
        liveBadge.textContent = 'AO VIVO NO TOTEM';
        titleH4.appendChild(liveBadge);
      }

      const domainLabel = atom.domain || atom.category || '';
      if (domainLabel) {
        const domainBadge = document.createElement('span');
        domainBadge.className = 'badge-pill';
        domainBadge.style.cssText = 'font-size: 10px; background: rgba(56, 189, 248, 0.15); color: var(--accent-cyan); margin-left: 6px;';
        domainBadge.textContent = domainLabel;
        titleH4.appendChild(domainBadge);
      }
      titleGroup.appendChild(titleH4);

      const subtitle = atom.subtitle || atom.scientific_name || atom.type_label || '';
      if (subtitle) {
        const subDiv = document.createElement('div');
        subDiv.className = 'atom-scientific';
        subDiv.textContent = subtitle;
        titleGroup.appendChild(subDiv);
      }

      const typeBadge = document.createElement('span');
      typeBadge.className = 'atom-type-badge';
      typeBadge.textContent = atom.type || 'entity';

      headerDiv.appendChild(titleGroup);
      headerDiv.appendChild(typeBadge);
      card.appendChild(headerDiv);

      // Facts
      if (atom.canonical_facts && atom.canonical_facts.length > 0) {
        const factsUl = document.createElement('ul');
        factsUl.className = 'atom-facts';
        atom.canonical_facts.forEach(f => {
          const li = document.createElement('li');
          li.className = 'atom-fact-item';
          const bullet = document.createElement('strong');
          bullet.textContent = '• ';
          li.appendChild(bullet);
          li.appendChild(document.createTextNode(f.statement || ''));
          factsUl.appendChild(li);
        });
        card.appendChild(factsUl);
      }

      // Metadata Chips
      if (atom.metadata && typeof atom.metadata === 'object') {
        const metaEntries = Object.entries(atom.metadata).slice(0, 3);
        if (metaEntries.length > 0) {
          const metaContainer = document.createElement('div');
          metaContainer.className = 'atom-meta-chips';
          metaContainer.style.cssText = 'display:flex; flex-wrap:wrap; gap:6px; margin: 8px 0;';
          metaEntries.forEach(([k, v]) => {
            const chip = document.createElement('span');
            chip.className = 'tag-mono';
            chip.style.cssText = 'font-size:0.75rem; background:rgba(255,255,255,0.06); padding:2px 8px; border-radius:4px;';
            const kStrong = document.createElement('strong');
            kStrong.style.color = 'var(--accent-cyan)';
            kStrong.textContent = `${k}: `;
            chip.appendChild(kStrong);
            chip.appendChild(document.createTextNode(String(v)));
            metaContainer.appendChild(chip);
          });
          card.appendChild(metaContainer);
        }
      }

      // Assets row
      const assetsRow = document.createElement('div');
      assetsRow.className = 'atom-assets-row';
      const imageCount = (atom.images ? atom.images.length : (atom.modalities?.image?.length || 0));
      const audioCount = (atom.audios ? atom.audios.length : (atom.modalities?.audio?.length || 0));
      if (imageCount > 0) {
        const imgTag = document.createElement('span');
        imgTag.className = 'asset-tag';
        imgTag.textContent = `🖼️ ${imageCount} imagem`;
        assetsRow.appendChild(imgTag);
      }
      if (audioCount > 0) {
        const audTag = document.createElement('span');
        audTag.className = 'asset-tag';
        audTag.textContent = `🎵 ${audioCount} áudio`;
        assetsRow.appendChild(audTag);
      }
      card.appendChild(assetsRow);

      // Actions row
      const actionsRow = document.createElement('div');
      actionsRow.className = 'atom-actions-row';

      const firstAudio = (atom.audios && atom.audios.length > 0) ? atom.audios[0] : (atom.modalities?.audio?.[0]);
      if (firstAudio) {
        const audioSrc = firstAudio.startsWith('assets/') ? firstAudio : 'assets/' + firstAudio;
        const playBtn = document.createElement('button');
        playBtn.className = 'btn btn-outline btn-sm btn-play-audio';
        playBtn.dataset.src = audioSrc;
        playBtn.textContent = '▶️ Ouvir';
        actionsRow.appendChild(playBtn);
      }

      const totemBtn = document.createElement('button');
      totemBtn.className = 'btn btn-outline btn-sm btn-kiosk-show';
      totemBtn.dataset.id = atom.content_id;
      totemBtn.title = 'Exibir imediatamente no totem';
      totemBtn.textContent = '📺 Totem';
      actionsRow.appendChild(totemBtn);

      const editBtn = document.createElement('button');
      editBtn.className = 'btn btn-secondary btn-sm btn-edit-atom';
      editBtn.dataset.id = atom.content_id;
      editBtn.textContent = '✏️ Editar';
      actionsRow.appendChild(editBtn);

      const delBtn = document.createElement('button');
      delBtn.className = 'btn btn-danger-outline btn-sm btn-del-atom';
      delBtn.dataset.id = atom.content_id;
      delBtn.dataset.title = atom.canonical_name || atom.title || '';
      delBtn.textContent = '🗑️';
      actionsRow.appendChild(delBtn);

      card.appendChild(actionsRow);
      container.appendChild(card);
    });

    // Attach actions
    container.querySelectorAll('.btn-kiosk-show').forEach(btn => {
      btn.addEventListener('click', async () => {
        const id = btn.getAttribute('data-id');
        try {
          btn.disabled = true;
          const res = await fetch('/api/kiosk/show', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ content_id: id })
          });
          const data = await res.json();
          btn.disabled = false;
          if (data.success) {
            showNotice('Totem Atualizado', `Exibindo átomo "${id}" no totem agora.`);
            fetchStatus();
          } else {
            showNotice('Falha no Totem', data.error || 'Totem não respondeu', true);
          }
        } catch (err) {
          btn.disabled = false;
          showNotice('Erro de Comunicação', err.message, true);
        }
      });
    });
    container.querySelectorAll('.btn-edit-atom').forEach(btn => {
      btn.addEventListener('click', () => {
        const id = btn.getAttribute('data-id');
        openAtomModalForEdit(id);
      });
    });

    container.querySelectorAll('.btn-del-atom').forEach(btn => {
      btn.addEventListener('click', async () => {
        const id = btn.getAttribute('data-id');
        const title = btn.getAttribute('data-title');
        if (!confirm(`Deseja realmente excluir o átomo "${title}" (${id})?`)) return;

        try {
          const res = await fetch(`/api/atoms/${encodeURIComponent(id)}`, { method: 'DELETE' });
          const data = await res.json();
          if (data.success) {
            showNotice('Átomo Excluído', `O átomo ${id} foi removido do rascunho.`);
            updateLifecycleUI(true);
            fetchCatalog();
          } else {
            showNotice('Erro ao Excluir', data.error || 'Erro desconhecido', true);
          }
        } catch (err) {
          showNotice('Erro de Comunicação', err.message, true);
        }
      });
    });

    container.querySelectorAll('.btn-play-audio').forEach(btn => {
      btn.addEventListener('click', () => {
        const src = btn.getAttribute('data-src');
        const audio = new Audio('/' + src);
        audio.play().catch(e => console.warn('Falha ao reproduzir áudio:', e));
      });
    });
  }

  function renderRelations(relations) {
    const container = document.getElementById('relations-grid');
    container.innerHTML = '';

    if (relations.length === 0) {
      container.innerHTML = '<p style="color:var(--text-dim); padding:20px;">Nenhuma relação ecológica cadastrada.</p>';
      return;
    }

    relations.forEach(rel => {
      const card = document.createElement('div');
      card.className = 'relation-card';

      const header = document.createElement('div');
      header.className = 'rel-header';
      const badge = document.createElement('span');
      badge.className = 'rel-badge';
      badge.textContent = rel.type || 'vínculo';
      const delBtn = document.createElement('button');
      delBtn.className = 'btn btn-danger-outline btn-sm btn-del-relation';
      delBtn.dataset.id = rel.relation_id;
      delBtn.textContent = '🗑️';
      header.appendChild(badge);
      header.appendChild(delBtn);

      const nodes = document.createElement('div');
      nodes.className = 'rel-nodes';
      const fromSpan = document.createElement('span');
      fromSpan.textContent = rel.from;
      const arrowSpan = document.createElement('span');
      arrowSpan.className = 'rel-arrow';
      arrowSpan.textContent = '➔';
      const toSpan = document.createElement('span');
      toSpan.textContent = rel.to;
      nodes.appendChild(fromSpan);
      nodes.appendChild(arrowSpan);
      nodes.appendChild(toSpan);

      const desc = document.createElement('p');
      desc.className = 'rel-desc';
      desc.textContent = rel.description || 'Vínculo ecossistêmico verificado.';

      card.appendChild(header);
      card.appendChild(nodes);
      card.appendChild(desc);
      container.appendChild(card);
    });

    container.querySelectorAll('.btn-del-relation').forEach(btn => {
      btn.addEventListener('click', async () => {
        const id = btn.getAttribute('data-id');
        if (!confirm(`Deseja remover esta relação ecológica?`)) return;
        try {
          const res = await fetch(`/api/relations/${encodeURIComponent(id)}`, { method: 'DELETE' });
          const data = await res.json();
          if (data.success) {
            showNotice('Relação Removida', 'Vínculo ecológico atualizado.');
            updateLifecycleUI(true);
            fetchCatalog();
          }
        } catch (err) {
          showNotice('Erro', err.message, true);
        }
      });
    });
  }

  function renderRecipes(recipes) {
    const container = document.getElementById('recipes-grid');
    container.innerHTML = '';

    recipes.forEach(rec => {
      const card = document.createElement('div');
      card.className = 'recipe-card';
      const title = document.createElement('h4');
      title.className = 'recipe-title';
      title.textContent = rec.name || rec.recipe_id;
      const desc = document.createElement('p');
      desc.className = 'recipe-desc';
      desc.textContent = rec.description || 'Receita contextual de engajamento do visitante.';
      const steps = document.createElement('div');
      steps.className = 'recipe-steps-count';
      steps.textContent = `Passos coreografados: ${rec.step_count || 1}`;

      card.appendChild(title);
      card.appendChild(desc);
      card.appendChild(steps);
      container.appendChild(card);
    });
  }

  function renderBundlesTable(bundles, activeBundle) {
    const tbody = document.getElementById('bundles-tbody');
    tbody.innerHTML = '';

    if (bundles.length === 0) {
      tbody.innerHTML = `<tr><td colspan="6" style="text-align:center; padding: 24px; color: var(--text-dim);">Nenhum bundle publicado em disco ainda.</td></tr>`;
      return;
    }

    bundles.forEach(b => {
      const isCurrent = activeBundle && (activeBundle.content_hash === b.content_hash || activeBundle.version === b.version);
      const tr = document.createElement('tr');
      const hashShort = b.content_hash ? b.content_hash.substring(0, 18) + '...' : '—';
      const dateStr = b.created_at ? new Date(b.created_at).toLocaleString('pt-BR') : '—';

      const tdVer = document.createElement('td');
      const verStrong = document.createElement('strong');
      verStrong.textContent = `v${b.version}`;
      tdVer.appendChild(verStrong);
      if (isCurrent) {
        const curBadge = document.createElement('span');
        curBadge.className = 'badge-pill badge-green';
        curBadge.style.marginLeft = '6px';
        curBadge.textContent = 'ATIVO';
        tdVer.appendChild(curBadge);
      }

      const tdRev = document.createElement('td');
      tdRev.textContent = `#${b.curation_revision}`;

      const tdTitle = document.createElement('td');
      tdTitle.textContent = b.title || b.bundle_id;

      const tdHash = document.createElement('td');
      const codeHash = document.createElement('code');
      codeHash.className = 'hash-code';
      codeHash.textContent = hashShort;
      tdHash.appendChild(codeHash);

      const tdDate = document.createElement('td');
      tdDate.textContent = dateStr;

      const tdAction = document.createElement('td');
      if (!isCurrent) {
        const rbBtn = document.createElement('button');
        rbBtn.className = 'btn btn-secondary btn-sm btn-rollback';
        rbBtn.dataset.target = `${b.bundle_id}-${b.version}`;
        rbBtn.textContent = 'Ativar / Rollback';
        tdAction.appendChild(rbBtn);
      } else {
        const curSpan = document.createElement('span');
        curSpan.style.cssText = 'color:var(--text-dim); font-size:0.8rem;';
        curSpan.textContent = 'Em Execução';
        tdAction.appendChild(curSpan);
      }

      tr.appendChild(tdVer);
      tr.appendChild(tdRev);
      tr.appendChild(tdTitle);
      tr.appendChild(tdHash);
      tr.appendChild(tdDate);
      tr.appendChild(tdAction);
      tbody.appendChild(tr);
    });

    tbody.querySelectorAll('.btn-rollback').forEach(btn => {
      btn.addEventListener('click', async () => {
        const target = btn.getAttribute('data-target');
        if (!confirm(`Deseja realizar o Rollback atômico para o bundle ${target}?`)) return;

        try {
          const res = await fetch('/api/rollback', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ target })
          });
          const resp = await res.json();
          if (resp.success) {
            showNotice('Rollback Concluído!', `Bundle ${target} ativado com sucesso no totem.`);
            fetchStatus();
            fetchCatalog();
          } else {
            showNotice('Falha no Rollback', resp.error || 'Erro desconhecido', true);
          }
        } catch (err) {
          showNotice('Erro de Comunicação', err.message, true);
        }
      });
    });
  }

  // Atom Modal & Form Handling
  const modalAtom = document.getElementById('modal-atom');
  const formAtom = document.getElementById('form-atom');
  const btnNewAtom = document.getElementById('btn-new-atom');
  const modalAtomClose = document.getElementById('modal-atom-close');
  const btnAtomCancel = document.getElementById('btn-atom-cancel');
  const factsContainer = document.getElementById('facts-container');
  const btnAddFact = document.getElementById('btn-add-fact');
  const metaPairsContainer = document.getElementById('meta-pairs-container');
  const btnAddMetaPair = document.getElementById('btn-add-meta-pair');

  function openAtomModalForCreate() {
    document.getElementById('modal-atom-title').textContent = 'Novo Átomo de Conteúdo';
    document.getElementById('atom-edit-mode').value = 'create';
    formAtom.reset();
    document.getElementById('atom-id').disabled = false;
    document.getElementById('preview-image-box').classList.add('hidden');
    document.getElementById('preview-audio-box').classList.add('hidden');
    factsContainer.innerHTML = '';
    addFactRow('', 'source_curadoria_local');
    if (metaPairsContainer) {
      metaPairsContainer.innerHTML = '';
      addMetaPairRow('', '');
    }
    modalAtom.classList.remove('hidden');
  }

  function openAtomModalForEdit(atomId) {
    const atom = (catalogData.atoms || []).find(a => a.content_id === atomId);
    if (!atom) return;

    document.getElementById('modal-atom-title').textContent = `Editar Átomo: ${atom.canonical_name || atom.title}`;
    document.getElementById('atom-edit-mode').value = 'edit';
    document.getElementById('atom-id').value = atom.content_id;
    document.getElementById('atom-id').disabled = true;
    document.getElementById('atom-type').value = atom.type || 'entity';
    document.getElementById('atom-subtype').value = atom.subtype || '';
    document.getElementById('atom-language').value = atom.language || 'pt-BR';
    document.getElementById('atom-summary').value = atom.summary || '';
    document.getElementById('atom-status').value = atom.lifecycle_status || 'draft';
    document.getElementById('atom-title').value = atom.title || '';
    document.getElementById('atom-canonical').value = atom.canonical_name || atom.subject?.canonical_name || '';
    document.getElementById('atom-scientific').value = atom.scientific_name || atom.subtitle || atom.subject?.scientific_name || '';
    document.getElementById('atom-type-label').value = atom.type_label || atom.subject?.type_label || '';
    document.getElementById('atom-themes').value = (atom.themes || []).join(', ');
    document.getElementById('atom-creator').value = atom.provenance?.creator || '';
    document.getElementById('atom-publisher').value = atom.provenance?.publisher || '';
    document.getElementById('atom-source').value = atom.provenance?.source_reference || '';
    document.getElementById('atom-license').value = atom.rights?.license || '';
    document.getElementById('atom-rights-holder').value = atom.rights?.rights_holder || '';
    document.getElementById('atom-attribution').value = atom.rights?.attribution || '';
    document.getElementById('atom-alt-text').value = atom.accessibility?.alt_text || '';
    document.getElementById('atom-transcript').value = atom.accessibility?.transcript || '';

    const imgPath = (atom.images && atom.images.length > 0) ? atom.images[0] : (atom.modalities?.image?.[0] || '');
    document.getElementById('atom-image-path').value = imgPath;
    if (imgPath) {
      document.getElementById('preview-image').src = '/' + imgPath;
      document.getElementById('preview-image-box').classList.remove('hidden');
    } else {
      document.getElementById('preview-image-box').classList.add('hidden');
    }

    const audPath = (atom.audios && atom.audios.length > 0) ? atom.audios[0] : (atom.modalities?.audio?.[0] || '');
    document.getElementById('atom-audio-path').value = audPath;
    if (audPath) {
      document.getElementById('preview-audio').src = '/' + audPath;
      document.getElementById('preview-audio-box').classList.remove('hidden');
    } else {
      document.getElementById('preview-audio-box').classList.add('hidden');
    }

    factsContainer.innerHTML = '';
    if (atom.canonical_facts && atom.canonical_facts.length > 0) {
      atom.canonical_facts.forEach(f => {
        addFactRow(f.statement, (f.source_ids || []).join(', '));
      });
    } else {
      addFactRow('', 'source_curadoria_local');
    }

    if (metaPairsContainer) {
      metaPairsContainer.innerHTML = '';
      if (atom.metadata && typeof atom.metadata === 'object' && Object.keys(atom.metadata).length > 0) {
        for (const [k, v] of Object.entries(atom.metadata)) {
          addMetaPairRow(k, v);
        }
      } else {
        addMetaPairRow('', '');
      }
    }

    modalAtom.classList.remove('hidden');
  }

  function addFactRow(statement = '', sources = '') {
    const row = document.createElement('div');
    row.className = 'fact-row';

    const inputsDiv = document.createElement('div');
    inputsDiv.className = 'fact-inputs';

    const ta = document.createElement('textarea');
    ta.className = 'input-textarea fact-statement';
    ta.rows = 2;
    ta.placeholder = 'Declaração factual sobre o átomo...';
    ta.value = statement;

    const inp = document.createElement('input');
    inp.type = 'text';
    inp.className = 'input-text fact-sources';
    inp.placeholder = 'Fontes de autoridade / Referências (ex: Museu, UFRGS 2024, IPHAN)';
    inp.value = sources;

    inputsDiv.appendChild(ta);
    inputsDiv.appendChild(inp);

    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'btn-del-fact';
    btn.title = 'Remover fato';
    btn.textContent = '×';
    btn.addEventListener('click', () => row.remove());

    row.appendChild(inputsDiv);
    row.appendChild(btn);
    factsContainer.appendChild(row);
  }

  function addMetaPairRow(key = '', value = '') {
    if (!metaPairsContainer) return;
    const row = document.createElement('div');
    row.className = 'meta-pair-row';

    const inputsDiv = document.createElement('div');
    inputsDiv.className = 'meta-pair-inputs';

    const keyInp = document.createElement('input');
    keyInp.type = 'text';
    keyInp.className = 'input-text meta-key';
    keyInp.placeholder = 'Campo (ex: Autor, Época, Dimensões, Instituição)';
    keyInp.value = key;
    keyInp.style.flex = '1';

    const valInp = document.createElement('input');
    valInp.type = 'text';
    valInp.className = 'input-text meta-val';
    valInp.placeholder = 'Valor correspondente';
    valInp.value = value;
    valInp.style.flex = '2';

    inputsDiv.appendChild(keyInp);
    inputsDiv.appendChild(valInp);

    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'btn-del-meta';
    btn.title = 'Remover campo';
    btn.textContent = '×';
    btn.addEventListener('click', () => row.remove());

    row.appendChild(inputsDiv);
    row.appendChild(btn);
    metaPairsContainer.appendChild(row);
  }

  btnAddFact.addEventListener('click', () => addFactRow());
  if (btnAddMetaPair) {
    btnAddMetaPair.addEventListener('click', () => addMetaPairRow('', ''));
  }
  btnNewAtom.addEventListener('click', openAtomModalForCreate);
  modalAtomClose.addEventListener('click', () => modalAtom.classList.add('hidden'));
  btnAtomCancel.addEventListener('click', () => modalAtom.classList.add('hidden'));

  // File Upload Handlers (Image & Audio)
  async function handleFileUpload(fileInput, folder, pathInputId, previewBoxId, previewMediaId) {
    const file = fileInput.files[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = async (e) => {
      const base64Data = e.target.result;
      try {
        const res = await fetch('/api/upload', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            folder: folder,
            filename: file.name,
            base64_data: base64Data
          })
        });
        const data = await res.json();
        if (data.success) {
          document.getElementById(pathInputId).value = data.path;
          const previewMedia = document.getElementById(previewMediaId);
          previewMedia.src = '/' + data.path;
          document.getElementById(previewBoxId).classList.remove('hidden');
          showNotice('Mídia Enviada!', `Arquivo ${file.name} salvo em ${data.path}.`);
        } else {
          showNotice('Erro no Upload', data.error || 'Falha ao salvar', true);
        }
      } catch (err) {
        showNotice('Falha de Rede', err.message, true);
      }
    };
    reader.readAsDataURL(file);
  }

  document.getElementById('upload-image-file').addEventListener('change', (e) => {
    handleFileUpload(e.target, 'images', 'atom-image-path', 'preview-image-box', 'preview-image');
  });

  document.getElementById('upload-audio-file').addEventListener('change', (e) => {
    handleFileUpload(e.target, 'audio', 'atom-audio-path', 'preview-audio-box', 'preview-audio');
  });

  const btnDeleteAtomImage = document.getElementById('btn-delete-atom-image');
  if (btnDeleteAtomImage) {
    btnDeleteAtomImage.addEventListener('click', async () => {
      const imgPath = document.getElementById('atom-image-path').value.trim();
      if (!imgPath) {
        document.getElementById('preview-image-box').classList.add('hidden');
        return;
      }

      const deleteFileOnDisk = confirm(`Deseja excluir a imagem deste átomo?\n\n• Clique em OK para apagar também o arquivo de mídia do disco (${imgPath})\n• Clique em Cancelar para apenas desvincular deste átomo`);

      if (deleteFileOnDisk) {
        try {
          const res = await fetch(`/api/media?path=${encodeURIComponent(imgPath)}`, { method: 'DELETE' });
          const data = await res.json();
          if (data.success) {
            showNotice('Imagem Excluída', `Arquivo ${imgPath} removido do repositório soberano de mídias.`);
            updateLifecycleUI(true);
            fetchMedia();
            fetchCatalog();
          } else {
            showNotice('Aviso na Exclusão', data.error || 'Não foi possível apagar o arquivo do disco', true);
          }
        } catch (err) {
          showNotice('Falha na Exclusão', err.message, true);
        }
      } else {
        showNotice('Imagem Desvinculada', 'A imagem foi desvinculada do formulário. Salve o átomo para confirmar.');
      }

      document.getElementById('atom-image-path').value = '';
      document.getElementById('preview-image').src = '';
      document.getElementById('preview-image-box').classList.add('hidden');
      document.getElementById('upload-image-file').value = '';
    });
  }

  const btnDeleteAtomAudio = document.getElementById('btn-delete-atom-audio');
  if (btnDeleteAtomAudio) {
    btnDeleteAtomAudio.addEventListener('click', async () => {
      const audPath = document.getElementById('atom-audio-path').value.trim();
      if (!audPath) {
        document.getElementById('preview-audio-box').classList.add('hidden');
        return;
      }

      const deleteFileOnDisk = confirm(`Deseja excluir o áudio deste átomo?\n\n• Clique em OK para apagar também o arquivo do disco (${audPath})\n• Clique em Cancelar para apenas desvincular do átomo`);

      if (deleteFileOnDisk) {
        try {
          const res = await fetch(`/api/media?path=${encodeURIComponent(audPath)}`, { method: 'DELETE' });
          const data = await res.json();
          if (data.success) {
            showNotice('Áudio Excluído', `Arquivo ${audPath} removido do repositório.`);
            updateLifecycleUI(true);
            fetchMedia();
            fetchCatalog();
          }
        } catch (err) {
          showNotice('Falha na Exclusão', err.message, true);
        }
      } else {
        showNotice('Áudio Desvinculado', 'O áudio foi desvinculado do formulário. Salve o átomo para confirmar.');
      }

      document.getElementById('atom-audio-path').value = '';
      document.getElementById('preview-audio').src = '';
      document.getElementById('preview-audio-box').classList.add('hidden');
      document.getElementById('upload-audio-file').value = '';
    });
  }

  // Save Atom (Create or Update)
  formAtom.addEventListener('submit', async (e) => {
    e.preventDefault();

    const mode = document.getElementById('atom-edit-mode').value;
    const contentId = document.getElementById('atom-id').value.trim();
    const type = document.getElementById('atom-type').value;
    const subtype = document.getElementById('atom-subtype').value.trim();
    const title = document.getElementById('atom-title').value.trim();
    const canonicalName = document.getElementById('atom-canonical').value.trim() || title;
    const scientificName = document.getElementById('atom-scientific').value.trim();
    const typeLabel = document.getElementById('atom-type-label').value.trim();
    const themesStr = document.getElementById('atom-themes').value.trim();
    const themes = themesStr ? themesStr.split(',').map(s => s.trim()).filter(Boolean) : ['geral'];

    const language = document.getElementById('atom-language').value.trim() || 'pt-BR';
    const summary = document.getElementById('atom-summary').value.trim();
    const lifecycleStatus = document.getElementById('atom-status').value;
    const subtitle = scientificName;

    // Collect facts
    const facts = [];
    document.querySelectorAll('.fact-row').forEach((row, idx) => {
      const stmt = row.querySelector('.fact-statement').value.trim();
      const srcs = row.querySelector('.fact-sources').value.trim();
      if (stmt) {
        facts.push({
          fact_id: `fact_${contentId || 'item'}_${idx + 1}`,
          statement: stmt,
          source_ids: srcs ? srcs.split(',').map(s => s.trim()).filter(Boolean) : ['source_curadoria_local'],
          confidence: 'reviewed'
        });
      }
    });

    // Collect custom structured metadata pairs
    const customMetadata = {};
    document.querySelectorAll('.meta-pair-row').forEach(row => {
      const k = row.querySelector('.meta-key').value.trim();
      const v = row.querySelector('.meta-val').value.trim();
      if (k && v) {
        customMetadata[k] = v;
      }
    });

    const imgPath = document.getElementById('atom-image-path').value.trim();
    const audPath = document.getElementById('atom-audio-path').value.trim();

    const payload = {
      schema_version: '0.2',
      content_id: contentId,
      type: type,
      subtype: subtype,
      title: title,
      summary: summary,
      language: language,
      lifecycle_status: lifecycleStatus,
      subtitle: subtitle,
      subject: {
        canonical_name: canonicalName,
        scientific_name: scientificName,
        type_label: typeLabel
      },
      metadata: customMetadata,
      themes: themes,
      canonical_facts: facts,
      modalities: {
        image: imgPath ? [imgPath] : [],
        audio: audPath ? [audPath] : []
      },
      supported_roles: ['ambient', 'attract', 'engage', 'deepen'],
      provenance: {
        creator: document.getElementById('atom-creator').value.trim(),
        publisher: document.getElementById('atom-publisher').value.trim(),
        source_reference: document.getElementById('atom-source').value.trim(),
        reviewed: lifecycleStatus === 'approved',
        created_at: mode === 'edit' ? ((catalogData.atoms || []).find(a => a.content_id === contentId)?.provenance?.created_at || new Date().toISOString()) : new Date().toISOString(),
        modified_at: new Date().toISOString()
      },
      rights: {
        license: document.getElementById('atom-license').value.trim(),
        rights_holder: document.getElementById('atom-rights-holder').value.trim(),
        attribution: document.getElementById('atom-attribution').value.trim()
      },
      accessibility: {
        alt_text: document.getElementById('atom-alt-text').value.trim(),
        transcript: document.getElementById('atom-transcript').value.trim()
      }
    };

    try {
      const url = mode === 'edit' ? `/api/atoms/${encodeURIComponent(contentId)}` : '/api/atoms';
      const method = mode === 'edit' ? 'PUT' : 'POST';

      const res = await fetch(url, {
        method: method,
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });
      const data = await res.json();

      if (data.success) {
        modalAtom.classList.add('hidden');
        showNotice('Átomo Salvo!', `Átomo ${data.content_id || contentId} salvo no rascunho. Estado editorial agora é DRAFT.`);
        updateLifecycleUI(true);
        fetchCatalog();
      } else {
        showNotice('Erro ao Salvar Átomo', data.error || 'Erro desconhecido', true);
      }
    } catch (err) {
      showNotice('Erro de Comunicação', err.message, true);
    }
  });

  // Relation Modal & Form Handling
  const modalRelation = document.getElementById('modal-relation');
  const formRelation = document.getElementById('form-relation');
  const btnNewRelation = document.getElementById('btn-new-relation');
  const modalRelationClose = document.getElementById('modal-relation-close');
  const btnRelCancel = document.getElementById('btn-rel-cancel');

  btnNewRelation?.addEventListener('click', () => {
    const selFrom = document.getElementById('rel-from');
    const selTo = document.getElementById('rel-to');
    selFrom.innerHTML = '';
    selTo.innerHTML = '';

    (catalogData.atoms || []).forEach(a => {
      const opt1 = document.createElement('option');
      opt1.value = a.content_id;
      opt1.textContent = `${a.canonical_name || a.title} (${a.content_id})`;
      selFrom.appendChild(opt1);

      const opt2 = document.createElement('option');
      opt2.value = a.content_id;
      opt2.textContent = `${a.canonical_name || a.title} (${a.content_id})`;
      selTo.appendChild(opt2);
    });

    formRelation.reset();
    modalRelation.classList.remove('hidden');
  });

  modalRelationClose?.addEventListener('click', () => modalRelation?.classList.add('hidden'));
  btnRelCancel?.addEventListener('click', () => modalRelation?.classList.add('hidden'));

  formRelation?.addEventListener('submit', async (e) => {
    e.preventDefault();
    const fromId = document.getElementById('rel-from').value;
    const toId = document.getElementById('rel-to').value;
    const type = document.getElementById('rel-type').value;
    const conf = parseFloat(document.getElementById('rel-confidence').value) || 1.0;
    const desc = document.getElementById('rel-desc').value.trim();

    try {
      const res = await fetch('/api/relations', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          relation_id: `rel_${fromId}_${toId}`,
          from: fromId,
          to: toId,
          type: type,
          confidence: conf,
          description: desc,
          source_ids: ['source_curadoria_local']
        })
      });
      const data = await res.json();
      if (data.success) {
        modalRelation.classList.add('hidden');
        showNotice('Relação Criada!', 'Nova relação ecológica cadastrada em rascunho.');
        updateLifecycleUI(true);
        fetchCatalog();
      } else {
        showNotice('Erro ao Salvar Relação', data.error || 'Erro', true);
      }
    } catch (err) {
      showNotice('Erro', err.message, true);
    }
  });

  // Action Buttons: Validate, Publish, Package Management, Reload
  const btnPublish = document.getElementById('btn-publish');
  if (btnPublish) {
    btnPublish.addEventListener('click', async () => {
      if (!confirm('Deseja empacotar, validar e publicar a versão ativa/rascunho no Totem?')) return;
      try {
        showNotice('Publicando no Totem…', 'Calculando hash determinístico e sincronizando Totem via IPC...');
        const res = await fetch('/api/publish', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ bundle_id: selectedPackageId || '' })
        });
        const data = await res.json();
        if (data.success) {
          showNotice('Publicação Concluída com Sucesso! 🚀', `Aplicação "${data.bundle_id}" (v${data.version}) publicada e ativada no Totem.`);
          await fetchStatus();
          await fetchCatalog();
          await fetchPackagePlans(data.bundle_id);
        } else {
          showNotice('Falha na Publicação', data.error || 'Erro desconhecido', true);
        }
      } catch (err) {
        showNotice('Erro de Comunicação', err.message, true);
      }
    });
  }

  document.getElementById('btn-validate').addEventListener('click', async () => {
    try {
      showNotice('Validando Catálogo...', 'Executando ContentValidator nativo em C++26...');
      const res = await fetch('/api/validate', { method: 'POST' });
      const data = await res.json();

      if (data.valid) {
        showNotice('Validação Concluída com Sucesso! ✓', 'Catálogo passou por 100% dos testes semânticos, relações, proveniências e integridade.');
      } else {
        const errs = (data.errors || []).map(e => `${e.item_id}: ${e.message}`).join('; ');
        showNotice('Inconsistências Encontradas', errs, true);
      }
    } catch (err) {
      showNotice('Erro ao Validar', err.message, true);
    }
  });

  document.getElementById('btn-open-packages').addEventListener('click', () => {
    document.querySelector('[data-tab="tab-bundles"]')?.click();
    document.getElementById('tab-bundles')?.scrollIntoView({ behavior: 'smooth', block: 'start' });
  });

  document.getElementById('btn-reload').addEventListener('click', async () => {
    try {
      const res = await fetch('/api/kiosk/reload', { method: 'POST' });
      const data = await res.json();
      if (data.success) {
        showNotice('Kiosk Recarregado', 'Sinal CONTENT_RELOAD enviado via socket Unix ao totem.');
      } else {
        showNotice('Aviso', 'Kiosk não respondeu no socket IPC. Verifique se o processo elo-kiosk está em execução.', true);
      }
    } catch (err) {
      showNotice('Erro de Comunicação', err.message, true);
    }
  });

  // Media & Assets Manager
  let mediaItems = [];
  let currentMediaFilter = 'all';

  async function fetchMedia() {
    try {
      const res = await fetch('/api/media');
      if (!res.ok) throw new Error('Media API error: ' + res.status);
      const data = await res.json();
      mediaItems = data.media || [];
      renderMedia(mediaItems, currentMediaFilter);
    } catch (err) {
      console.warn('Erro ao carregar mídias:', err);
    }
  }

  function formatBytes(bytes) {
    if (!bytes || bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i];
  }

  function renderMedia(items, filter = 'all') {
    const grid = document.getElementById('media-grid');
    if (!grid) return;
    grid.innerHTML = '';

    const filtered = items.filter(item => {
      if (filter === 'all') return true;
      return item.type === filter;
    });

    if (filtered.length === 0) {
      grid.innerHTML = `<p style="grid-column: 1/-1; text-align: center; color: var(--text-dim); padding: 40px;">Nenhum arquivo de mídia encontrado nesta categoria.</p>`;
      return;
    }

    filtered.forEach(item => {
      const card = document.createElement('div');
      card.className = 'media-card';

      const thumbContainer = document.createElement('div');
      thumbContainer.className = 'media-thumb-container';
      if (item.type === 'image') {
        const img = document.createElement('img');
        img.className = 'media-thumb';
        img.src = '/' + encodeURI(item.path);
        img.alt = item.filename;
        img.loading = 'lazy';
        thumbContainer.appendChild(img);
      } else {
        const audioBox = document.createElement('div');
        audioBox.className = 'media-audio-box';
        const iconSpan = document.createElement('span');
        iconSpan.style.fontSize = '2.2rem';
        iconSpan.textContent = '🎵';
        const audio = document.createElement('audio');
        audio.controls = true;
        audio.src = '/' + encodeURI(item.path);
        audio.style.cssText = 'width: 100%; max-height: 40px;';
        audioBox.appendChild(iconSpan);
        audioBox.appendChild(audio);
        thumbContainer.appendChild(audioBox);
      }
      card.appendChild(thumbContainer);

      const bodyDiv = document.createElement('div');
      bodyDiv.className = 'media-body';

      const h4Name = document.createElement('h4');
      h4Name.className = 'media-name';
      h4Name.title = item.filename;
      h4Name.textContent = item.filename;
      bodyDiv.appendChild(h4Name);

      const pathSpan = document.createElement('span');
      pathSpan.className = 'media-path-text';
      pathSpan.textContent = item.path;
      bodyDiv.appendChild(pathSpan);

      const metaRow = document.createElement('div');
      metaRow.className = 'media-meta-row';
      const sizeSpan = document.createElement('span');
      sizeSpan.textContent = formatBytes(item.size_bytes);
      const typeSpan = document.createElement('span');
      typeSpan.textContent = item.type === 'image' ? '🖼️ Imagem' : '🎵 Áudio';
      metaRow.appendChild(sizeSpan);
      metaRow.appendChild(typeSpan);
      bodyDiv.appendChild(metaRow);

      const isUsed = item.used_by && item.used_by.length > 0;
      const usageBadge = document.createElement('span');
      if (isUsed) {
        usageBadge.className = 'media-badge media-badge-used';
        usageBadge.title = item.used_by.join(', ');
        const usedPrefix = item.used_by.slice(0, 2).join(', ');
        const extra = item.used_by.length > 2 ? ` +${item.used_by.length - 2}` : '';
        usageBadge.textContent = `✓ Vinculado: ${usedPrefix}${extra}`;
      } else {
        usageBadge.className = 'media-badge media-badge-orphan';
        usageBadge.textContent = '⚪ Mídia não vinculada';
      }
      bodyDiv.appendChild(usageBadge);

      const actionsDiv = document.createElement('div');
      actionsDiv.className = 'media-actions';
      const copyBtn = document.createElement('button');
      copyBtn.className = 'btn btn-outline btn-xs btn-copy-path';
      copyBtn.dataset.path = item.path;
      copyBtn.textContent = '📋 Copiar';

      const delBtn = document.createElement('button');
      delBtn.className = 'btn btn-danger-outline btn-xs btn-delete-media';
      delBtn.dataset.path = item.path;
      delBtn.dataset.name = item.filename;
      delBtn.textContent = '🗑️ Excluir';

      actionsDiv.appendChild(copyBtn);
      actionsDiv.appendChild(delBtn);
      bodyDiv.appendChild(actionsDiv);

      card.appendChild(bodyDiv);
      grid.appendChild(card);
    });

    // Event listeners dos cards de mídia
    grid.querySelectorAll('.btn-copy-path').forEach(btn => {
      btn.addEventListener('click', () => {
        const p = btn.getAttribute('data-path');
        navigator.clipboard.writeText(p).then(() => {
          showNotice('Caminho Copiado!', `Caminho "${p}" copiado para a área de transferência.`);
        });
      });
    });

    grid.querySelectorAll('.btn-delete-media').forEach(btn => {
      btn.addEventListener('click', async () => {
        const p = btn.getAttribute('data-path');
        const name = btn.getAttribute('data-name');
        if (!confirm(`Deseja realmente EXCLUIR a mídia "${name}"?\n\n• O arquivo será excluído permanentemente do totem.\n• Quaisquer vínculos em átomos existentes serão limpos automaticamente.`)) {
          return;
        }

        try {
          const res = await fetch(`/api/media?path=${encodeURIComponent(p)}`, { method: 'DELETE' });
          const data = await res.json();
          if (data.success) {
            showNotice('Mídia Excluída!', `Arquivo "${name}" excluído com sucesso do totem.`);
            updateLifecycleUI(true);
            fetchMedia();
            fetchCatalog();
          } else {
            showNotice('Erro ao Excluir', data.error || 'Falha na exclusão', true);
          }
        } catch (err) {
          showNotice('Falha na Exclusão', err.message, true);
        }
      });
    });
  }

  // Filtros de mídia
  document.querySelectorAll('.filter-chip').forEach(chip => {
    chip.addEventListener('click', () => {
      document.querySelectorAll('.filter-chip').forEach(c => c.classList.remove('active'));
      chip.classList.add('active');
      currentMediaFilter = chip.getAttribute('data-filter') || 'all';
      renderMedia(mediaItems, currentMediaFilter);
    });
  });

  // Upload direto na aba de mídias
  const directMediaUpload = document.getElementById('direct-media-upload');
  if (directMediaUpload) {
    directMediaUpload.addEventListener('change', async (e) => {
      const file = e.target.files[0];
      if (!file) return;

      const folder = file.type.startsWith('audio') ? 'audio' : 'images';
      const reader = new FileReader();
      reader.onload = async (evt) => {
        const base64Data = evt.target.result;
        try {
          showNotice('Enviando Mídia...', `Processando upload de ${file.name}...`);
          const res = await fetch('/api/upload', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              folder: folder,
              filename: file.name,
              base64_data: base64Data
            })
          });
          const data = await res.json();
          if (data.success) {
            showNotice('Upload Realizado!', `Arquivo ${file.name} salvo com sucesso em ${data.path}.`);
            updateLifecycleUI(true);
            fetchMedia();
          } else {
            showNotice('Erro no Upload', data.error || 'Falha ao salvar', true);
          }
        } catch (err) {
          showNotice('Falha de Rede', err.message, true);
        }
      };
      reader.readAsDataURL(file);
      directMediaUpload.value = '';
    });
  }

  const btnKioskAdvance = document.getElementById('btn-kiosk-advance');
  if (btnKioskAdvance) {
    btnKioskAdvance.addEventListener('click', async () => {
      try {
        btnKioskAdvance.disabled = true;
        const res = await fetch('/api/kiosk/advance', { method: 'POST' });
        const data = await res.json();
        btnKioskAdvance.disabled = false;
        if (data.success) {
          showNotice('Totem Avançado', 'Comando de avanço enviado ao totem com sucesso.');
          setTimeout(fetchStatus, 400);
        } else {
          showNotice('Falha no Totem', data.error || 'Totem não respondeu', true);
        }
      } catch (err) {
        btnKioskAdvance.disabled = false;
        showNotice('Erro de Comunicação', err.message, true);
      }
    });
  }

  // Kiosk Start / Stop / Restart Controls
  async function performKioskAction(action, label) {
    try {
      showNotice('Controle do Kiosk', `Executando: ${label}...`);
      const res = await fetch(`/api/kiosk/${action}`, { method: 'POST' });
      const data = await res.json();
      if (data.success) {
        showNotice('Sucesso', data.message || `Ação "${label}" executada.`);
      } else {
        showNotice('Atenção / Erro', data.error || data.message || 'Falha ao executar ação no Kiosk', true);
      }
      setTimeout(fetchStatus, 800);
      setTimeout(fetchStatus, 2000);
    } catch (err) {
      showNotice('Erro de Comunicação', err.message, true);
    }
  }

  ['btn-kiosk-start', 'btn-kiosk-start-main', 'btn-ipc-start'].forEach(id => {
    const el = document.getElementById(id);
    if (el) el.addEventListener('click', () => performKioskAction('start', 'Iniciar Totem'));
  });

  ['btn-kiosk-restart', 'btn-ipc-restart'].forEach(id => {
    const el = document.getElementById(id);
    if (el) el.addEventListener('click', () => performKioskAction('restart', 'Reiniciar Totem'));
  });

  ['btn-kiosk-stop', 'btn-ipc-stop'].forEach(id => {
    const el = document.getElementById(id);
    if (el) el.addEventListener('click', () => {
      if (confirm('Deseja realmente encerrar a execução do totem (kiosk)?')) {
        performKioskAction('stop', 'Encerrar Totem');
      }
    });
  });

  // Package Meta & Publish Actions
  const formPkgMeta = document.getElementById('form-package-meta');
  const btnSavePkg = document.getElementById('btn-save-package-meta');
  const btnPublishPkg = document.getElementById('btn-publish-package');

  async function savePackagePlan(andActivate = false) {
      const title = document.getElementById('pkg-title').value.trim();
      const bundleId = document.getElementById('pkg-id').value.trim();
      const version = document.getElementById('pkg-version').value.trim();
      const theme = document.getElementById('pkg-theme').value.trim();
      const desc = document.getElementById('pkg-desc').value.trim();
      const domainCategory = document.getElementById('pkg-domain').value.trim();
      const targetAudience = document.getElementById('pkg-audience').value.trim();

      if (!title || !bundleId || !version) {
        showNotice('Campos Obrigatórios', 'Preencha nome, identificador e versão da aplicação.', true);
        return false;
      }

      try {
        showNotice('Salvando Aplicação…', `Registrando "${title}" (${bundleId})...`);
        const res = await fetch('/api/packages', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            schema_version: '0.2',
            bundle_id: bundleId,
            version: version,
            title: title,
            default_theme: theme,
            description: desc,
            domain_category: domainCategory,
            target_audience: targetAudience,
            atom_ids: Array.from(packageAtomIds)
          })
        });
        const data = await res.json();
        if (data.success) {
          if (andActivate) {
            showNotice('Publicando no Totem…', `Publicando e ativando "${title}"...`);
            const actRes = await fetch(`/api/packages/${encodeURIComponent(bundleId)}/activate`, { method: 'POST' });
            const actData = await actRes.json();
            if (actData.success) {
              showNotice('Aplicação Publicada e Ativa! 🚀', `Aplicação "${title}" (v${actData.version || version}) está ativa no Totem.`);
            } else {
              showNotice('Salvo, mas falha ao ativar', actData.error || 'Erro na ativação', true);
            }
          } else {
            showNotice('Aplicação Salva!', `A aplicação "${title}" (${version}) foi salva no banco editorial.`);
          }
          await fetchStatus();
          await fetchCatalog();
          await fetchPackagePlans(bundleId);
          await loadPackagePlan(bundleId);
          return true;
        } else {
          showNotice('Erro ao Salvar', data.error || 'Falha', true);
          return false;
        }
      } catch (err) {
        showNotice('Falha de Rede', err.message, true);
        return false;
      }
  }

  if (formPkgMeta) {
    formPkgMeta.addEventListener('submit', (e) => {
      e.preventDefault();
      savePackagePlan(false);
    });
  }

  if (btnSavePkg) {
    btnSavePkg.addEventListener('click', () => savePackagePlan(false));
  }

  if (btnPublishPkg) {
    btnPublishPkg.addEventListener('click', () => savePackagePlan(true));
  }

  const btnRefreshAnalytics = document.getElementById('btn-refresh-analytics');
  if (btnRefreshAnalytics) {
    btnRefreshAnalytics.addEventListener('click', () => {
      fetchAnalytics();
      showNotice('Métricas Atualizadas', 'Dados analíticos agregados e grafo de fluxo sincronizados.');
    });
  }
  document.getElementById('analytics-app-filter')?.addEventListener('change', fetchAnalytics);

  // Initial load
  fetchStatus().then(() => fetchManifest()).then(() => fetchPackagePlans());
  fetchCatalog();
  fetchMedia();
  fetchAnalytics();
  setInterval(fetchStatus, 4000);
});
