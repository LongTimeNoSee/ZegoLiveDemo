package com.zego.livedemo3.ui;

import android.Manifest;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.text.TextUtils;
import android.view.SurfaceView;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.ToggleButton;

import com.zego.livedemo3.BizApiManager;
import com.zego.livedemo3.PublishActivity;
import com.zego.livedemo3.R;
import com.zego.livedemo3.ZegoApiManager;
import com.zego.livedemo3.base.AbsBaseFragment;
import com.zego.livedemo3.utils.ZegoAVKitUtil;
import com.zego.zegoavkit2.ZegoAVKit;
import com.zego.zegoavkit2.ZegoAVKitCommon;

import butterknife.Bind;
import butterknife.OnClick;

/**
 * Copyright © 2016 Zego. All rights reserved.
 * des:
 */
public class PublishFragment extends AbsBaseFragment {

    @Bind(R.id.tb_enable_front_cam)
    public ToggleButton tbEnableFrontCam;

    @Bind(R.id.tb_enable_torch)
    public ToggleButton tbEnableTorch;


    @Bind(R.id.sp_filters)
    public Spinner spFilters;

    @Bind(R.id.sp_beauties)
    public Spinner spBeauties;

    @Bind(R.id.et_publish_title)
    public EditText etPublishTitle;

    @Bind(R.id.sv_preview)
    public SurfaceView svPreview;


    private int mSelectedBeauty = 0;

    private int mSelectedFilter = 0;

    private ZegoAVKit mZegoAVKit;

    private boolean mHasBeenCreated = false;

    private boolean mIsVisiableToUser = false;

    public static PublishFragment newInstance() {
        return new PublishFragment();
    }

    @Override
    protected int getContentViewLayout() {
        return R.layout.fragment_publish;
    }

    @Override
    protected void initExtraData() {

    }

    @Override
    protected void initVariables() {
        mZegoAVKit = ZegoApiManager.getInstance().getZegoAVKit();
    }

    @Override
    protected void initViews() {
        ArrayAdapter<String> beautyAdapter = new ArrayAdapter<>(mParentActivity, R.layout.item_spinner, mResources.getStringArray(R.array.beauties));
        spBeauties.setAdapter(beautyAdapter);
        spBeauties.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                mSelectedBeauty = position;
                mZegoAVKit.enableBeautifying(ZegoAVKitUtil.getZegoBeauty(mSelectedBeauty));
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });

        ArrayAdapter<String> filterAdapter = new ArrayAdapter<>(mParentActivity, R.layout.item_spinner, mResources.getStringArray(R.array.filters));
        spFilters.setAdapter(filterAdapter);
        spFilters.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                mSelectedFilter = position;
                mZegoAVKit.setFilter(ZegoAVKitUtil.getZegoFilter(mSelectedFilter));
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });

        tbEnableFrontCam.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                mZegoAVKit.setFrontCam(isChecked);
                if (isChecked) {
                    tbEnableTorch.setEnabled(false);
                } else {
                    tbEnableTorch.setEnabled(true);
                }
            }
        });

        if (tbEnableFrontCam.isChecked()) {
            tbEnableTorch.setEnabled(false);
        }
        tbEnableTorch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                mZegoAVKit.enableTorch(isChecked);
            }
        });
    }

    @Override
    protected void loadData() {
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mHasBeenCreated) {
            if (mIsVisiableToUser) {
               startPreview();
            }
        } else {
            mHasBeenCreated = true;
        }

    }

    @Override
    public void onPause() {
        super.onPause();
    }

    @OnClick(R.id.btn_start_publish)
    public void startPublishing() {
        String publishTitle = etPublishTitle.getText().toString();
        if (TextUtils.isEmpty(publishTitle)) {
            publishTitle = BizApiManager.getInstance().getBizUser().userName;
        }

        stopPreview();

        hideInputWindow();
        PublishActivity.actionStart(mParentActivity, publishTitle, tbEnableFrontCam.isChecked(), tbEnableTorch.isChecked(), mSelectedBeauty, mSelectedFilter);

    }

    @OnClick(R.id.main_content)
    public void hideInputWindow() {
        InputMethodManager imm = (InputMethodManager) mParentActivity.getSystemService(Context.INPUT_METHOD_SERVICE);
        imm.hideSoftInputFromWindow(mParentActivity.getCurrentFocus().getWindowToken(), InputMethodManager.HIDE_NOT_ALWAYS);
    }

    @Override
    public void setUserVisibleHint(boolean isVisibleToUser) {
        super.setUserVisibleHint(isVisibleToUser);
        mIsVisiableToUser = isVisibleToUser;
        if (mHasBeenCreated) {
            if (isVisibleToUser) {

                // 6.0及以上的系统需要在运行时申请CAMERA RECORD_AUDIO权限
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                    if (ContextCompat.checkSelfPermission(mParentActivity, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED
                            || ContextCompat.checkSelfPermission(mParentActivity, Manifest.permission.RECORD_AUDIO) != PackageManager.PERMISSION_GRANTED) {
                        ActivityCompat.requestPermissions(mParentActivity, new String[]{
                                Manifest.permission.CAMERA, Manifest.permission.RECORD_AUDIO}, 101);
                    } else {
                        startPreview();
                    }
                } else {
                    startPreview();
                }
            } else {
                stopPreview();
            }
        }
    }

    private void startPreview(){
        mZegoAVKit.setFrontCam(tbEnableFrontCam.isChecked());
        mZegoAVKit.setLocalView(svPreview);
        mZegoAVKit.setLocalViewMode(ZegoAVKitCommon.ZegoVideoViewMode.ScaleAspectFill);
        mZegoAVKit.startPreview();
        svPreview.setVisibility(View.VISIBLE);
    }

    private void stopPreview(){
        svPreview.setVisibility(View.INVISIBLE);
        mZegoAVKit.stopPreview();
        mZegoAVKit.setLocalView(null);
    }
}