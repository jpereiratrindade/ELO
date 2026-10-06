document.addEventListener('DOMContentLoaded', () => {
  let catalogData = { atoms: [], relations: [], recipes: [] };
  let currentStatus = null;

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
    } catch (err) {
      console.warn('Erro ao carregar catálogo:', err);
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

      let audioButtonHtml = '';
      if (atom.audios && atom.audios.length > 0) {
        const audioSrc = atom.audios[0].startsWith('assets/') ? atom.audios[0] : 'assets/' + atom.audios[0];
        audioButtonHtml = `<button class="btn btn-outline btn-sm btn-play-audio" data-src="${audioSrc}">▶️ Ouvir</button>`;
      }

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
        <div class="atom-actions-row">
          ${audioButtonHtml}
          <button class="btn btn-secondary btn-sm btn-edit-atom" data-id="${atom.content_id}">✏️ Editar</button>
          <button class="btn btn-danger-outline btn-sm btn-del-atom" data-id="${atom.content_id}" data-title="${atom.canonical_name || atom.title}">🗑️</button>
        </div>
      `;
      container.appendChild(card);
    });

    // Attach actions
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

  function openAtomModalForCreate() {
    document.getElementById('modal-atom-title').textContent = 'Novo Átomo de Conteúdo';
    document.getElementById('atom-edit-mode').value = 'create';
    formAtom.reset();
    document.getElementById('atom-id').disabled = false;
    document.getElementById('preview-image-box').classList.add('hidden');
    document.getElementById('preview-audio-box').classList.add('hidden');
    factsContainer.innerHTML = '';
    addFactRow('', 'source_ufrgs_2023');
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
    document.getElementById('atom-title').value = atom.title || '';
    document.getElementById('atom-canonical').value = atom.canonical_name || '';
    document.getElementById('atom-scientific').value = atom.scientific_name || '';
    document.getElementById('atom-type-label').value = atom.type_label || '';
    document.getElementById('atom-themes').value = (atom.themes || []).join(', ');

    const imgPath = (atom.images && atom.images.length > 0) ? atom.images[0] : '';
    document.getElementById('atom-image-path').value = imgPath;
    if (imgPath) {
      document.getElementById('preview-image').src = '/' + imgPath;
      document.getElementById('preview-image-box').classList.remove('hidden');
    } else {
      document.getElementById('preview-image-box').classList.add('hidden');
    }

    const audPath = (atom.audios && atom.audios.length > 0) ? atom.audios[0] : '';
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
      addFactRow('', 'source_ufrgs_2023');
    }

    modalAtom.classList.remove('hidden');
  }

  function addFactRow(statement = '', sources = '') {
    const row = document.createElement('div');
    row.className = 'fact-row';
    row.innerHTML = `
      <div class="fact-inputs">
        <textarea class="input-textarea fact-statement" rows="2" placeholder="Declaração factual sobre o átomo...">${statement}</textarea>
        <input type="text" class="input-text fact-sources" placeholder="Fontes (ex: source_icmbio_2018, source_ufrgs_2023)" value="${sources}">
      </div>
      <button type="button" class="btn-del-fact" title="Remover fato">&times;</button>
    `;
    row.querySelector('.btn-del-fact').addEventListener('click', () => row.remove());
    factsContainer.appendChild(row);
  }

  btnAddFact.addEventListener('click', () => addFactRow());
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
    const title = document.getElementById('atom-title').value.trim();
    const canonicalName = document.getElementById('atom-canonical').value.trim() || title;
    const scientificName = document.getElementById('atom-scientific').value.trim();
    const typeLabel = document.getElementById('atom-type-label').value.trim();
    const themesStr = document.getElementById('atom-themes').value.trim();
    const themes = themesStr ? themesStr.split(',').map(s => s.trim()).filter(Boolean) : ['pampa'];

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

    const imgPath = document.getElementById('atom-image-path').value.trim();
    const audPath = document.getElementById('atom-audio-path').value.trim();

    const payload = {
      schema_version: '0.1',
      content_id: contentId,
      type: type,
      subtype: 'custom',
      title: title,
      subject: {
        canonical_name: canonicalName,
        scientific_name: scientificName,
        type_label: typeLabel
      },
      themes: themes,
      canonical_facts: facts,
      modalities: {
        image: imgPath ? [imgPath] : [],
        audio: audPath ? [audPath] : []
      },
      supported_roles: ['ambient', 'attract', 'engage', 'deepen'],
      provenance: { reviewed: true }
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

  btnNewRelation.addEventListener('click', () => {
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

  modalRelationClose.addEventListener('click', () => modalRelation.classList.add('hidden'));
  btnRelCancel.addEventListener('click', () => modalRelation.classList.add('hidden'));

  formRelation.addEventListener('submit', async (e) => {
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

  // Initial load
  fetchStatus();
  fetchCatalog();
  fetchMedia();
  setInterval(fetchStatus, 4000);
});
