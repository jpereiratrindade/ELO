document.addEventListener('DOMContentLoaded', () => {
  // Tab switching
  const tabButtons = document.querySelectorAll('.tab-btn');
  const tabPanes = document.querySelectorAll('.tab-pane');

  tabButtons.forEach(btn => {
    btn.addEventListener('click', () => {
      tabButtons.forEach(b => b.classList.remove('active'));
      tabPanes.forEach(p => p.classList.remove('active'));

      btn.classList.add('active');
      const target = document.getElementById(btn.getAttribute('data-tab'));
      if (target) target.classList.add('active');
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

  // State fetchers
  async function fetchStatus() {
    try {
      const res = await fetch('/api/status');
      if (!res.ok) throw new Error('Status HTTP error: ' + res.status);
      const data = await res.json();

      // Kiosk Status Pill
      const kioskPill = document.getElementById('kiosk-status-pill');
      const kioskText = document.getElementById('kiosk-status-text');
      kioskPill.classList.remove('status-checking', 'status-online', 'status-offline');
      if (data.kiosk_online) {
        kioskPill.classList.add('status-online');
        kioskText.textContent = 'Kiosk: ONLINE (IPC OK)';
      } else {
        kioskPill.classList.add('status-offline');
        kioskText.textContent = 'Kiosk: AGUARDANDO IPC';
      }

      // Socket path in IPC tab
      document.getElementById('ipc-socket-path').textContent = data.control_socket || '/run/elo/control.sock';
      const connState = document.getElementById('ipc-conn-state');
      connState.textContent = data.kiosk_online ? 'Conectado ao elo-kiosk' : 'Kiosk offline ou dormindo';
      connState.className = data.kiosk_online ? 'badge-pill badge-green' : 'badge-pill';

      // Active Bundle Card
      if (data.active_bundle) {
        document.getElementById('active-bundle-title').textContent = data.active_bundle.title || data.active_bundle.bundle_id;
        document.getElementById('active-bundle-desc').textContent = data.active_bundle.description || '';
        document.getElementById('active-bundle-hash').textContent = data.active_bundle.content_hash || 'Sem hash';
        document.getElementById('stat-version').textContent = 'v' + data.active_bundle.version;
        document.getElementById('stat-revision').textContent = '#' + data.active_bundle.curation_revision;
      }

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

      document.getElementById('stat-atoms').textContent = data.atoms ? data.atoms.length : 0;
      document.getElementById('stat-recipes').textContent = data.recipes ? data.recipes.length : 0;

      renderAtoms(data.atoms || []);
      renderRecipes(data.recipes || []);
    } catch (err) {
      console.warn('Erro ao carregar catálogo:', err);
    }
  }

  function renderAtoms(atoms) {
    const container = document.getElementById('atoms-grid');
    container.innerHTML = '';

    atoms.forEach(atom => {
      const card = document.createElement('div');
      card.className = 'atom-card';

      let factsHtml = '';
      if (atom.canonical_facts && atom.canonical_facts.length > 0) {
        factsHtml = `<ul class="atom-facts">` +
          atom.canonical_facts.map(f => `<li class="atom-fact-item">${f.statement}</li>`).join('') +
          `</ul>`;
      }

      let assetsHtml = '<div class="atom-assets-row">';
      if (atom.images && atom.images.length > 0) {
        assetsHtml += `<span class="asset-tag">🖼️ ${atom.images.length} imagem</span>`;
      }
      if (atom.audios && atom.audios.length > 0) {
        assetsHtml += `<span class="asset-tag">🎵 ${atom.audios.length} áudio</span>`;
      }
      assetsHtml += '</div>';

      card.innerHTML = `
        <div class="atom-header">
          <div>
            <h4 class="atom-title">${atom.canonical_name || atom.title}</h4>
            <div class="atom-scientific">${atom.scientific_name || atom.type_label || ''}</div>
          </div>
          <span class="atom-type-badge">${atom.type}</span>
        </div>
        ${factsHtml}
        ${assetsHtml}
      `;
      container.appendChild(card);
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

    // Attach rollback handlers
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

  // Action Buttons
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
        showNotice('Bundle Publicado e Ativo!', `Versão ${data.version} ativada. Hash: ${data.content_hash}. Kiosk recarregado a quente.`);
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

  // Initial load
  fetchStatus();
  fetchCatalog();
  setInterval(fetchStatus, 4000);
});
