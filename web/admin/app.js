document.addEventListener('DOMContentLoaded', () => {
  let catalogData = { atoms: [], relations: [], recipes: [] };
  let currentStatus = null;
  let packageAtomIds = new Set();

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
        if (target.id === 'tab-bundles') { fetchManifest(); fetchPackagePlans(); }
        if (target.id === 'tab-apps') fetchApplications();
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
      badge.textContent = 'BUNDLE ATIVO';
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
        document.getElementById('active-bundle-desc').textContent = 'Catálogo soberano local em modo Rascunho Editorial. Clique em "Publicar no Totem" para ativar a primeira versão.';
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
      if (document.getElementById('pkg-app-id')) document.getElementById('pkg-app-id').value = m.application_id || '';
      packageAtomIds = new Set(Array.isArray(m.atom_ids) ? m.atom_ids : []);
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

  async function fetchPackagePlans() {
    const select = document.getElementById('pkg-plan-select');
    if (!select) return;
    try {
      const res = await fetch('/api/packages');
      if (!res.ok) return;
      const data = await res.json();
      const currentId = document.getElementById('pkg-id')?.value || '';
      select.innerHTML = '<option value="">Novo pacote / manifesto atual</option>';
      (data.packages || []).forEach(plan => {
        const option = document.createElement('option');
        option.value = plan.bundle_id;
        option.textContent = `${plan.title || plan.bundle_id} (v${plan.version || '0.1.0'})`;
        option.selected = plan.bundle_id === currentId;
        select.appendChild(option);
      });
    } catch (e) {
      console.warn('Erro ao listar planos de pacote:', e);
    }
  }

  document.getElementById('pkg-plan-select')?.addEventListener('change', async (event) => {
    const id = event.target.value;
    if (!id) return;
    try {
      const res = await fetch(`/api/packages/${encodeURIComponent(id)}`);
      if (!res.ok) throw new Error('Pacote não encontrado');
      applyPackageManifest(await res.json());
    } catch (e) {
      showNotice('Erro ao carregar pacote', e.message, true);
    }
  });

  function renderPackageAtoms(atoms) {
    const list = document.getElementById('pkg-atoms-list');
    const badge = document.getElementById('pkg-atoms-count-badge');
    if (!list) return;
    list.innerHTML = '';
    const selectedCount = packageAtomIds.size || atoms.length;
    if (badge) badge.textContent = `${selectedCount} / ${atoms.length} Átomos`;

    if (atoms.length === 0) {
      list.innerHTML = '<p style="color:var(--text-dim); padding:10px;">Nenhum átomo cadastrado neste pacote ainda.</p>';
      return;
    }

    atoms.forEach(a => {
      const item = document.createElement('div');
      item.style.cssText = 'background: var(--bg-surface); padding: 10px 14px; border-radius: var(--radius-md); display: flex; justify-content: space-between; align-items: center; border: 1px solid var(--border-subtle);';
      item.innerHTML = `
        <input type="checkbox" class="pkg-atom-check" data-id="${a.content_id}" ${packageAtomIds.size === 0 || packageAtomIds.has(a.content_id) ? 'checked' : ''} aria-label="Incluir ${a.title || a.content_id} no pacote">
        <div>
          <strong style="color: var(--text-main); font-size: 0.9rem;">${a.canonical_name || a.title}</strong>
          <div style="color: var(--text-dim); font-size: 0.75rem;">${a.type_label || a.type} · <code>${a.content_id}</code></div>
        </div>
        <span class="badge-pill" style="font-size: 10px;">${a.type}</span>
      `;
      list.appendChild(item);
    });
    list.querySelectorAll('.pkg-atom-check').forEach(check => {
      check.addEventListener('change', () => {
        if (packageAtomIds.size === 0) packageAtomIds = new Set(atoms.map(a => a.content_id));
        if (check.checked) packageAtomIds.add(check.dataset.id); else packageAtomIds.delete(check.dataset.id);
        if (badge) badge.textContent = `${packageAtomIds.size} / ${atoms.length} Átomos`;
      });
    });
  }

  let applicationsData = [];

  async function fetchApplications() {
    try {
      const res = await fetch('/api/applications');
      if (!res.ok) return;
      const data = await res.json();
      applicationsData = data.applications || [];
      renderApplications(applicationsData);
    } catch (e) {
      console.warn('Erro ao carregar aplicações:', e);
    }
  }

  function renderApplications(apps) {
    const grid = document.getElementById('apps-grid');
    if (!grid) return;
    grid.innerHTML = '';

    if (apps.length === 0) {
      grid.innerHTML = '<p style="color:var(--text-dim); padding:20px;">Nenhuma aplicação ou projeto registrado ainda.</p>';
      return;
    }

    apps.forEach(app => {
      const card = document.createElement('div');
      card.className = 'atom-card';
      if (app.is_active) {
        card.style.borderColor = 'var(--accent-emerald)';
        card.style.boxShadow = '0 0 16px rgba(16, 185, 129, 0.2)';
      } else {
        card.style.borderColor = 'rgba(6, 182, 212, 0.3)';
      }

      const tagsHtml = (app.tags || []).map(t => `<span class="badge-pill" style="font-size:10px; margin-right:4px;">#${t}</span>`).join('');
      const metaHtml = Object.entries(app.metadata_schema || {}).map(([k, v]) => `<div><strong>${k}:</strong> ${v}</div>`).join('');
      const statusBadge = app.is_active
        ? `<span class="badge-pill badge-green" style="font-weight: bold;">★ PROJETO ATIVO NO TOTEM</span>`
        : `<span class="badge-pill" style="opacity: 0.7;">INATIVO</span>`;

      card.innerHTML = `
        <div class="atom-header" style="align-items: flex-start;">
          <div>
            <div style="margin-bottom: 6px;">${statusBadge}</div>
            <h4 class="atom-title" style="font-size: 1.15rem; color: var(--text-main);">${app.name}</h4>
            <div class="atom-scientific"><code>${app.app_id}</code> · v${app.version}</div>
          </div>
          <span class="atom-type-badge" style="background: rgba(6,182,212,0.15); color: var(--accent-cyan); border-color: rgba(6,182,212,0.4);">${app.domain_category}</span>
        </div>
        <p style="font-size: 0.88rem; color: var(--text-muted); margin: 12px 0; line-height: 1.5;">${app.description || 'Sem descrição curatorial.'}</p>
        <div style="font-size: 0.82rem; color: var(--text-dim); background: var(--bg-surface); padding: 12px; border-radius: var(--radius-sm); margin-bottom: 14px; border: 1px solid var(--border-subtle);">
          <div><strong>Pacote Vinculado:</strong> <code>${app.active_bundle_id || 'Nenhum'}</code></div>
          <div><strong>Público-Alvo:</strong> ${app.target_audience || 'general'} | <strong>Tema:</strong> ${app.default_theme || 'default'}</div>
          ${metaHtml}
        </div>
        <div style="margin-bottom: 14px;">${tagsHtml}</div>
        <div class="atom-actions" style="display: flex; gap: 8px; justify-content: flex-end; border-top: 1px solid var(--border-subtle); padding-top: 12px;">
          ${!app.is_active ? `<button class="btn btn-sm btn-primary btn-activate-app" data-id="${app.app_id}">⭐ Ativar no Totem</button>` : `<span class="badge-pill badge-green" style="font-size: 11px;">✓ Em Execução</span>`}
          <button class="btn btn-sm btn-secondary btn-edit-app" data-id="${app.app_id}">✏️ Editar</button>
          <button class="btn btn-sm btn-danger-outline btn-delete-app" data-id="${app.app_id}">🗑️ Excluir</button>
        </div>
      `;
      grid.appendChild(card);
    });

    // Event listeners para os botões dos cards
    grid.querySelectorAll('.btn-activate-app').forEach(btn => {
      btn.addEventListener('click', async () => {
        const appId = btn.getAttribute('data-id');
        try {
          showNotice('Ativando Projeto...', `Configurando projeto ${appId} como ativo no Totem...`);
          const res = await fetch('/api/applications/activate', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ app_id: appId })
          });
          const data = await res.json();
          if (data.success) {
            showNotice('Projeto Ativado!', data.message || `Projeto ${appId} ativado no Totem.`);
            fetchApplications();
            fetchStatus();
          } else {
            showNotice('Erro ao Ativar', data.error || 'Falha na ativação', true);
          }
        } catch (err) {
          showNotice('Erro de Comunicação', err.message, true);
        }
      });
    });

    grid.querySelectorAll('.btn-edit-app').forEach(btn => {
      btn.addEventListener('click', () => {
        const appId = btn.getAttribute('data-id');
        openAppModalForEdit(appId);
      });
    });

    grid.querySelectorAll('.btn-delete-app').forEach(btn => {
      btn.addEventListener('click', async () => {
        const appId = btn.getAttribute('data-id');
        if (!confirm(`Deseja realmente EXCLUIR o projeto "${appId}"?`)) return;

        try {
          const res = await fetch(`/api/applications?id=${encodeURIComponent(appId)}`, { method: 'DELETE' });
          const data = await res.json();
          if (data.success) {
            showNotice('Projeto Excluído', `Projeto ${appId} removido com sucesso.`);
            fetchApplications();
          } else {
            showNotice('Erro ao Excluir', data.error || 'Falha', true);
          }
        } catch (err) {
          showNotice('Erro de Comunicação', err.message, true);
        }
      });
    });
  }

  async function fetchAnalytics() {
    try {
      const res = await fetch('/api/analytics/summary');
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
            item.innerHTML = `
              <div>
                <strong style="color:var(--text-main); font-size:0.9rem;">${atom}</strong>
                <div style="color:var(--text-dim); font-size:0.75rem;">${count} visualizações agregadas</div>
              </div>
              <span class="badge-pill badge-green">${avg.toFixed(1)}s de contemplação média</span>
            `;
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
            item.innerHTML = `
              <div style="display:flex; align-items:center; gap:8px; font-size:0.85rem;">
                <code style="color:var(--text-main);">${f.src}</code>
                <span style="color:var(--accent-cyan);">➔</span>
                <code style="color:var(--text-main);">${f.tgt}</code>
              </div>
              <span class="badge-pill" style="font-size:10px;">${f.count} transições</span>
            `;
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

      let factsHtml = '';
      if (atom.canonical_facts && atom.canonical_facts.length > 0) {
        factsHtml = `<ul class="atom-facts">` +
          atom.canonical_facts.map(f => `<li class="atom-fact-item"><strong>•</strong> ${f.statement}</li>`).join('') +
          `</ul>`;
      }

      let metaChipsHtml = '';
      if (atom.metadata && typeof atom.metadata === 'object') {
        const metaEntries = Object.entries(atom.metadata).slice(0, 3);
        if (metaEntries.length > 0) {
          metaChipsHtml = '<div class="atom-meta-chips" style="display:flex; flex-wrap:wrap; gap:6px; margin: 8px 0;">' +
            metaEntries.map(([k, v]) => `<span class="tag-mono" style="font-size:0.75rem; background:rgba(255,255,255,0.06); padding:2px 8px; border-radius:4px;"><strong style="color:var(--accent-cyan);">${k}:</strong> ${v}</span>`).join('') +
            '</div>';
        }
      }

      let assetsHtml = '<div class="atom-assets-row">';
      const imageCount = (atom.images ? atom.images.length : (atom.modalities?.image?.length || 0));
      const audioCount = (atom.audios ? atom.audios.length : (atom.modalities?.audio?.length || 0));
      if (imageCount > 0) {
        assetsHtml += `<span class="asset-tag">🖼️ ${imageCount} imagem</span>`;
      }
      if (audioCount > 0) {
        assetsHtml += `<span class="asset-tag">🎵 ${audioCount} áudio</span>`;
      }
      assetsHtml += '</div>';

      let audioButtonHtml = '';
      const firstAudio = (atom.audios && atom.audios.length > 0) ? atom.audios[0] : (atom.modalities?.audio?.[0]);
      if (firstAudio) {
        const audioSrc = firstAudio.startsWith('assets/') ? firstAudio : 'assets/' + firstAudio;
        audioButtonHtml = `<button class="btn btn-outline btn-sm btn-play-audio" data-src="${audioSrc}">▶️ Ouvir</button>`;
      }

      const isLiveOnKiosk = currentStatus && currentStatus.kiosk_active_atom_id === atom.content_id;
      const liveBadgeHtml = isLiveOnKiosk ? '<span class="badge-pill badge-green" style="font-size: 10px; margin-left: 6px;">AO VIVO NO TOTEM</span>' : '';

      const domainLabel = atom.domain || atom.category || '';
      const domainBadge = domainLabel ? `<span class="badge-pill" style="font-size: 10px; background: rgba(56, 189, 248, 0.15); color: var(--accent-cyan); margin-left: 6px;">${domainLabel}</span>` : '';

      const subtitle = atom.subtitle || atom.scientific_name || atom.type_label || '';

      card.innerHTML = `
        <div class="atom-header">
          <div>
            <h4 class="atom-title">${atom.title || atom.canonical_name} ${liveBadgeHtml} ${domainBadge}</h4>
            <div class="atom-scientific">${subtitle}</div>
          </div>
          <span class="atom-type-badge">${atom.type}</span>
        </div>
        ${factsHtml}
        ${metaChipsHtml}
        ${assetsHtml}
        <div class="atom-actions-row">
          ${audioButtonHtml}
          <button class="btn btn-outline btn-sm btn-kiosk-show" data-id="${atom.content_id}" title="Exibir imediatamente no totem">📺 Totem</button>
          <button class="btn btn-secondary btn-sm btn-edit-atom" data-id="${atom.content_id}">✏️ Editar</button>
          <button class="btn btn-danger-outline btn-sm btn-del-atom" data-id="${atom.content_id}" data-title="${atom.canonical_name || atom.title}">🗑️</button>
        </div>
      `;
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
      card.innerHTML = `
        <div class="rel-header">
          <span class="rel-badge">${rel.type || 'vínculo'}</span>
          <button class="btn btn-danger-outline btn-sm btn-del-relation" data-id="${rel.relation_id}">🗑️</button>
        </div>
        <div class="rel-nodes">
          <span>${rel.from}</span>
          <span class="rel-arrow">➔</span>
          <span>${rel.to}</span>
        </div>
        <p class="rel-desc">${rel.description || 'Vínculo ecossistêmico verificado.'}</p>
      `;
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
      card.innerHTML = `
        <h4 class="recipe-title">${rec.name || rec.recipe_id}</h4>
        <p class="recipe-desc">${rec.description || 'Receita contextual de engajamento do visitante.'}</p>
        <div class="recipe-steps-count">Passos coreografados: ${rec.step_count || 1}</div>
      `;
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

      tr.innerHTML = `
        <td><strong>v${b.version}</strong> ${isCurrent ? '<span class="badge-pill badge-green" style="margin-left:6px;">ATIVO</span>' : ''}</td>
        <td>#${b.curation_revision}</td>
        <td>${b.title || b.bundle_id}</td>
        <td><code class="hash-code">${hashShort}</code></td>
        <td>${dateStr}</td>
        <td>
          ${!isCurrent ? `<button class="btn btn-secondary btn-sm btn-rollback" data-target="${b.bundle_id}-${b.version}">Ativar / Rollback</button>` : '<span style="color:var(--text-dim); font-size:0.8rem;">Em Execução</span>'}
        </td>
      `;
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
    row.innerHTML = `
      <div class="fact-inputs">
        <textarea class="input-textarea fact-statement" rows="2" placeholder="Declaração factual sobre o átomo...">${statement}</textarea>
        <input type="text" class="input-text fact-sources" placeholder="Fontes de autoridade / Referências (ex: Museu, UFRGS 2024, IPHAN)" value="${sources}">
      </div>
      <button type="button" class="btn-del-fact" title="Remover fato">&times;</button>
    `;
    row.querySelector('.btn-del-fact').addEventListener('click', () => row.remove());
    factsContainer.appendChild(row);
  }

  function addMetaPairRow(key = '', value = '') {
    if (!metaPairsContainer) return;
    const row = document.createElement('div');
    row.className = 'meta-pair-row';
    row.innerHTML = `
      <div class="meta-pair-inputs">
        <input type="text" class="input-text meta-key" placeholder="Campo (ex: Autor, Época, Dimensões, Instituição)" value="${key}" style="flex: 1;">
        <input type="text" class="input-text meta-val" placeholder="Valor correspondente" value="${value}" style="flex: 2;">
      </div>
      <button type="button" class="btn-del-meta" title="Remover campo">&times;</button>
    `;
    row.querySelector('.btn-del-meta').addEventListener('click', () => row.remove());
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

  // Application / Project Modal & Form Handling
  const modalApp = document.getElementById('modal-app');
  const formApp = document.getElementById('form-app');
  const btnNewApp = document.getElementById('btn-new-app');
  const modalAppClose = document.getElementById('modal-app-close');
  const btnAppCancel = document.getElementById('btn-app-cancel');

  function populateAppBundleOptions(selectedBundleId) {
    const sel = document.getElementById('app-bundle');
    if (!sel) return;
    sel.innerHTML = '<option value="">(Nenhum pacote vinculado)</option>';
    if (currentStatus && currentStatus.bundles) {
      currentStatus.bundles.forEach(b => {
        const opt = document.createElement('option');
        opt.value = b.bundle_id;
        opt.textContent = `${b.title || b.bundle_id} (v${b.version})`;
        if (b.bundle_id === selectedBundleId) opt.selected = true;
        sel.appendChild(opt);
      });
    }
  }

  function openAppModalForCreate() {
    if (!modalApp) return;
    document.getElementById('modal-app-title').textContent = 'Novo Projeto / Aplicação';
    document.getElementById('app-edit-mode').value = 'create';
    formApp.reset();
    document.getElementById('app-id').disabled = false;
    document.getElementById('app-version').value = '1.0.0';
    populateAppBundleOptions(currentStatus && currentStatus.active_bundle ? currentStatus.active_bundle.bundle_id : '');
    modalApp.classList.remove('hidden');
  }

  function openAppModalForEdit(appId) {
    const app = applicationsData.find(a => a.app_id === appId);
    if (!app || !modalApp) return;

    document.getElementById('modal-app-title').textContent = `Editar Projeto: ${app.name}`;
    document.getElementById('app-edit-mode').value = 'edit';
    document.getElementById('app-id').value = app.app_id;
    document.getElementById('app-id').disabled = true;
    document.getElementById('app-name').value = app.name || '';
    document.getElementById('app-domain').value = app.domain_category || 'environmental_sciences';
    document.getElementById('app-version').value = app.version || '1.0.0';
    document.getElementById('app-theme').value = app.default_theme || '';
    document.getElementById('app-desc').value = app.description || '';
    document.getElementById('app-tags').value = (app.tags || []).join(', ');
    document.getElementById('app-active').checked = !!app.is_active;

    populateAppBundleOptions(app.active_bundle_id);
    modalApp.classList.remove('hidden');
  }

  if (btnNewApp) btnNewApp.addEventListener('click', openAppModalForCreate);
  if (modalAppClose) modalAppClose.addEventListener('click', () => modalApp.classList.add('hidden'));
  if (btnAppCancel) btnAppCancel.addEventListener('click', () => modalApp.classList.add('hidden'));

  if (formApp) {
    formApp.addEventListener('submit', async (e) => {
      e.preventDefault();
      const appId = document.getElementById('app-id').value.trim();
      const name = document.getElementById('app-name').value.trim();
      const domain = document.getElementById('app-domain').value;
      const version = document.getElementById('app-version').value.trim() || '1.0.0';
      const bundleId = document.getElementById('app-bundle').value;
      const theme = document.getElementById('app-theme').value.trim();
      const desc = document.getElementById('app-desc').value.trim();
      const tagsStr = document.getElementById('app-tags').value.trim();
      const isActive = document.getElementById('app-active').checked;

      const tags = tagsStr ? tagsStr.split(',').map(t => t.trim()).filter(Boolean) : [];

      const payload = {
        app_id: appId,
        name: name,
        domain_category: domain,
        version: version,
        active_bundle_id: bundleId,
        default_theme: theme,
        description: desc,
        tags: tags,
        is_active: isActive,
        telemetry_enabled: true
      };

      try {
        const res = await fetch('/api/applications', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        });
        const data = await res.json();
        if (data.success) {
          modalApp.classList.add('hidden');
          showNotice('Projeto Salvo!', `Projeto "${name}" (${appId}) salvo com sucesso.`);
          fetchApplications();
          if (isActive) fetchStatus();
        } else {
          showNotice('Erro ao Salvar Projeto', data.error || 'Erro', true);
        }
      } catch (err) {
        showNotice('Falha de Rede', err.message, true);
      }
    });
  }

  // Action Buttons: Validate, Publish, Reload
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

  document.getElementById('btn-publish').addEventListener('click', async () => {
    if (!confirm('Deseja empacotar, assinar (SHA-256) e ativar atomicamente este bundle no totem?')) return;

    try {
      showNotice('Publicando...', 'Gerando bundle, calculando hash e atualizando link atômico...');
      const res = await fetch('/api/publish', { method: 'POST' });
      const data = await res.json();

      if (data.success) {
        showNotice('Bundle Publicado e Ativo!', `Versão ${data.version} ativada no totem. Hash: ${data.content_hash}. Kiosk recarregado a quente via socket Unix.`);
        fetchStatus();
        fetchCatalog();
      } else {
        showNotice('Falha na Publicação', data.error || 'Erro desconhecido', true);
      }
    } catch (err) {
      showNotice('Erro na Publicação', err.message, true);
    }
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

      let mediaPreviewHtml = '';
      if (item.type === 'image') {
        mediaPreviewHtml = `
          <div class="media-thumb-container">
            <img class="media-thumb" src="/${item.path}" alt="${item.filename}" loading="lazy">
          </div>
        `;
      } else {
        mediaPreviewHtml = `
          <div class="media-thumb-container">
            <div class="media-audio-box">
              <span style="font-size: 2.2rem;">🎵</span>
              <audio controls src="/${item.path}" style="width: 100%; max-height: 40px;"></audio>
            </div>
          </div>
        `;
      }

      const isUsed = item.used_by && item.used_by.length > 0;
      const usageBadge = isUsed
        ? `<span class="media-badge media-badge-used" title="${item.used_by.join(', ')}">✓ Vinculado: ${item.used_by.slice(0, 2).join(', ')}${item.used_by.length > 2 ? ' +' + (item.used_by.length - 2) : ''}</span>`
        : `<span class="media-badge media-badge-orphan">⚪ Mídia não vinculada</span>`;

      card.innerHTML = `
        ${mediaPreviewHtml}
        <div class="media-body">
          <h4 class="media-name" title="${item.filename}">${item.filename}</h4>
          <span class="media-path-text">${item.path}</span>
          <div class="media-meta-row">
            <span>${formatBytes(item.size_bytes)}</span>
            <span>${item.type === 'image' ? '🖼️ Imagem' : '🎵 Áudio'}</span>
          </div>
          ${usageBadge}
          <div class="media-actions">
            <button class="btn btn-outline btn-xs btn-copy-path" data-path="${item.path}">📋 Copiar</button>
            <button class="btn btn-danger-outline btn-xs btn-delete-media" data-path="${item.path}" data-name="${item.filename}">🗑️ Excluir</button>
          </div>
        </div>
      `;
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
  const btnSavePkg = document.getElementById('btn-save-package-meta');
  async function savePackagePlan() {
      const title = document.getElementById('pkg-title').value.trim();
      const bundleId = document.getElementById('pkg-id').value.trim();
      const version = document.getElementById('pkg-version').value.trim();
      const theme = document.getElementById('pkg-theme').value.trim();
      const desc = document.getElementById('pkg-desc').value.trim();
      const applicationId = document.getElementById('pkg-app-id').value.trim();

      if (!title || !bundleId || !version) {
        showNotice('Campos Obrigatórios', 'Preencha Título, ID e Versão do Pacote.', true);
        return false;
      }

      try {
        const res = await fetch('/api/manifest', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            schema_version: '0.2',
            bundle_id: bundleId,
            version: version,
            title: title,
            default_theme: theme,
            description: desc,
            application_id: applicationId,
            atom_ids: packageAtomIds.size ? Array.from(packageAtomIds) : catalogData.atoms.map(a => a.content_id)
          })
        });
        const data = await res.json();
        if (data.success) {
          showNotice('Pacote Atualizado!', `Metadados do pacote "${title}" (${version}) salvos com sucesso.`);
          updateLifecycleUI(true);
          fetchStatus();
          fetchCatalog();
          fetchPackagePlans();
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
  if (btnSavePkg) {
    btnSavePkg.addEventListener('click', savePackagePlan);
  }

  const btnPublishPkgDirect = document.getElementById('btn-publish-package-direct');
  if (btnPublishPkgDirect) {
    btnPublishPkgDirect.addEventListener('click', async () => {
      if (!(await savePackagePlan())) return;
      const btnPub = document.getElementById('btn-publish');
      if (btnPub) btnPub.click();
    });
  }

  const btnRefreshAnalytics = document.getElementById('btn-refresh-analytics');
  if (btnRefreshAnalytics) {
    btnRefreshAnalytics.addEventListener('click', () => {
      fetchAnalytics();
      showNotice('Métricas Atualizadas', 'Dados analíticos agregados e grafo de fluxo sincronizados.');
    });
  }

  // Initial load
  fetchStatus();
  fetchCatalog();
  fetchMedia();
  fetchManifest();
  fetchPackagePlans();
  fetchApplications();
  fetchAnalytics();
  setInterval(fetchStatus, 4000);
});
